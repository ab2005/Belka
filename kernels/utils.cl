#define PIPELINE_DEPTH 14

#include "types.h"

__kernel void packBayer(__global short *input, __write_only image2d_t output) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    int p1 = (x + y * IMAGE_WIDTH) * 2;
    int p2 = p1 + IMAGE_WIDTH;
        
    float4 out = (float4)(input[p1], input[p1 + 1], input[p2], input[p2 + 1]) / 65535.f;
    write_imagef(output, (int2)(x, y), out);
}

/**
 * Performs zigzap packing, depth clipping-scaling and level adjusments.
 *      bb - normalized black pedestal,
 *      wg - gain factor
 */
// TODO pass wg scaled to 65536.f
// TODO pass black pedestal as parameter
__kernel void packAdjustLevels(__global ushort *in, __write_only image2d_t out, int4 bb, float4 wg) {
    int2 pos = (int2)(get_global_id(0), get_global_id(1));

    int p1 = (pos.x + pos.y * IMAGE_WIDTH) * 2;
    int p2 = p1 + IMAGE_WIDTH;
    
    int4 pix = (int4)(in[p1], in[p1 + 1], in[p2], in[p2 + 1]);
    pix = clamp(pix - bb, 0, 1 << IMAGE_DEPTH);
    pix <<= (PIPELINE_DEPTH - IMAGE_DEPTH) ;
    
    write_imagef(out, pos, (convert_float4(pix) * wg) / 65535.f);
}

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

inline bool isMedian(float val, float8 vec) {
    float8 v = (float8)val;
    float8 right = convert_float8(isgreater(v, vec));
    float8 left = convert_float8(isless(v, vec));
    
    float8 sum = right - left;

    return dot(sum.hi, sum.lo) == 0.f;
}

inline float getMedian(float val, float8 vec) {
    if (isMedian(val, vec)) return val;
    
    return -1.f;
}

__kernel void median3x3(__read_only image2d_t in, __write_only image2d_t out) {
    int x = get_global_id(0);
    int y = get_global_id(1);
        
    float4 p1 = read_imagef(in, sampler, (int2)(x - 1, y - 1));
    float4 p2 = read_imagef(in, sampler, (int2)(x    , y - 1));
    float4 p3 = read_imagef(in, sampler, (int2)(x + 1, y - 1));
    float4 p4 = read_imagef(in, sampler, (int2)(x - 1, y    ));
    float4 p5 = read_imagef(in, sampler, (int2)(x    , y    ));
    float4 p6 = read_imagef(in, sampler, (int2)(x + 1, y    ));
    float4 p7 = read_imagef(in, sampler, (int2)(x - 1, y + 1));
    float4 p8 = read_imagef(in, sampler, (int2)(x    , y + 1));
    float4 p9 = read_imagef(in, sampler, (int2)(x + 1, y + 1));

    float8 v1 = (float8)(p2.x, p3.x, p4.x, p5.x, p6.x, p7.x, p8.x, p9.x);
    float8 v2 = (float8)(p2.y, p3.y, p4.y, p5.y, p6.y, p7.y, p8.y, p9.y);
    float8 v3 = (float8)(p2.w, p3.w, p4.w, p5.w, p6.w, p7.w, p8.w, p9.w);
    float8 v4 = (float8)(p2.z, p3.z, p4.z, p5.z, p6.z, p7.z, p8.z, p9.z);
    
    float f1 = getMedian(p1.x, v1);
    float f2 = getMedian(p1.y, v2);
    float f3 = getMedian(p1.w, v3);
    float f4 = getMedian(p1.z, v4);
    
    write_imagef(out, (int2)(x, y), (float4)(f1, f2, f3, f4));
}

/*
 Copyright (c) 2011 Michael Zucchi
 
 This file is part of socles, an OpenCL image processing library.
 
 socles is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 socles is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with socles.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 Median filters
 */

// 84uS
#define cas(a, b)                               \
do {                                    \
float x = a;                    \
int c = a > b;                  \
a = c ? b : a;                  \
b = c ? x : b;                  \
} while (0)

#define cas2(a, b)                              \
do {                                            \
float4 x = a;                           \
int4 c = a> b;                          \
a.s01 = select(b, a, c).s01;            \
b.s01 = select(x, b, c).s01;            \
} while (0)

// 175
#define cas3(a, b)                              \
do {                                            \
float4 x = a;                           \
int4 c = a> b;                          \
a.s012 = select(b, a, c).s012;          \
b.s012 = select(x, b, c).s012;          \
} while (0)

// 238
#define cas4(a, b)                              \
do {                                    \
float4 x = a;                   \
int4 c = a > b;                 \
a = select(b, a, c);            \
b = select(x, b, c);            \
} while (0)


// 3x3 median filter
// uses a sorting network to sort entirely in registers with no branches
// sorting network from knuth volume 3, S5.3.4
kernel void
median3x3_r(image2d_t src, write_only image2d_t dst) {
    int gx = get_global_id(0), gy = get_global_id(1);
    const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    
    if ((gx >= get_image_width(dst)) | (gy >= get_image_height(dst)))
        return;
    
    float s0 = read_imagef(src, smp, (int2) { gx-1, gy-1 }).s0;
    float s1 = read_imagef(src, smp, (int2) { gx, gy-1 }).s0;
    float s2 = read_imagef(src, smp, (int2) { gx+1, gy-1 }).s0;
    float s3 = read_imagef(src, smp, (int2) { gx-1, gy }).s0;
    float s4 = read_imagef(src, smp, (int2) { gx, gy }).s0;
    float s5 = read_imagef(src, smp, (int2) { gx+1, gy }).s0;
    float s6 = read_imagef(src, smp, (int2) { gx-1, gy+1 }).s0;
    float s7 = read_imagef(src, smp, (int2) { gx, gy+1 }).s0;
    float s8 = read_imagef(src, smp, (int2) { gx+1, gy+1 }).s0;
    
    // stage0
    cas(s1, s2);
    cas(s4, s5);
    cas(s7, s8);
    
    // 1
    cas(s0, s1);
    cas(s3, s4);
    cas(s6, s7);
    
    // 2
    cas(s1, s2);
    cas(s4, s5);
    cas(s7, s8);
    
    // 3/4
    cas(s3, s6);
    cas(s4, s7);
    cas(s5, s8);
    cas(s0, s3);
    
    cas(s1, s4);
    cas(s2, s5);
    cas(s3, s6);
    
    //      cas(s5, s8); // not needed for median
    cas(s4, s7);
    cas(s1, s3);
    
    cas(s2, s6);
    //      cas(s5, s7); // not needed for median
    cas(s2, s3);
    cas(s4, s6);
    
    cas(s3, s4);
    //      cas(s5, s6); // not needed for median
    
    write_imagef(dst, (int2) (gx, gy), (float4)s4);
}

// rgb version
kernel void
median3x3_rgb(image2d_t src, write_only image2d_t dst) {
    int gx = get_global_id(0), gy = get_global_id(1);
    const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    
    if ((gx >= get_image_width(dst)) | (gy >= get_image_height(dst)))
        return;
    
    float4 s0 = read_imagef(src, smp, (int2) { gx-1, gy-1 });
    float4 s1 = read_imagef(src, smp, (int2) { gx, gy-1 });
    float4 s2 = read_imagef(src, smp, (int2) { gx+1, gy-1 });
    float4 s3 = read_imagef(src, smp, (int2) { gx-1, gy });
    float4 s4 = read_imagef(src, smp, (int2) { gx, gy });
    float4 s5 = read_imagef(src, smp, (int2) { gx+1, gy });
    float4 s6 = read_imagef(src, smp, (int2) { gx-1, gy+1 });
    float4 s7 = read_imagef(src, smp, (int2) { gx, gy+1 });
    float4 s8 = read_imagef(src, smp, (int2) { gx+1, gy+1 });
    
    // stage0
    cas3(s1, s2);
    cas3(s4, s5);
    cas3(s7, s8);
    
    // 1
    cas3(s0, s1);
    cas3(s3, s4);
    cas3(s6, s7);
    
    // 2
    cas3(s1, s2);
    cas3(s4, s5);
    cas3(s7, s8);
    
    // 3/4
    cas3(s3, s6);
    cas3(s4, s7);
    cas3(s5, s8);
    cas3(s0, s3);
    
    cas3(s1, s4);
    cas3(s2, s5);
    cas3(s3, s6);
    
    //      cas3(s5, s8); // not needed for median
    cas3(s4, s7);
    cas3(s1, s3);
    
    cas3(s2, s6);
    //      cas3(s5, s7); // not needed for median
    cas3(s2, s3);
    cas3(s4, s6);
    
    cas3(s3, s4);
    //      cas3(s5, s6); // not needed for median
    
    write_imagef(dst, (int2) (gx, gy), s4);
}


