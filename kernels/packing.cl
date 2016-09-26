#include "rccb.h"

#define SCALE (float4)(1.f / (1 << IMAGE_DEPTH))

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
/**
 * Zigzag packing and level adjusments.
 *      bb - normalized black pedestals,
 *      wg - gain factors
 *      bt - black offset
 */
kernel void packAdjustLevels(global ushort *in, write_only image2d_t out, int4 bb, float4 wg, float bt, read_only image2d_t temp, int tcnt) {
    int x = get_global_id(0) * 2;
    int y = get_global_id(1);
    int off = (y * IMAGE_WIDTH + x) * 2;

    float4 l10 = convert_float4(vload4(0, in + off));
    float4 l20 = convert_float4(vload4(0, in + off + IMAGE_WIDTH));

    // Level adjustments
    float4 fbb = convert_float4(bb);
    float4 p1 = ((float4)(l10.xy, l20.xy) - fbb) * wg + (float4)bt;
    float4 p2 = ((float4)(l10.zw, l20.zw) - fbb) * wg + (float4)bt;

    // Gamma encoding
    p1 = clamp(pow(p1 * SCALE , 1.f / 2.2f), 0.f, 0.999f);
    p2 = clamp(pow(p2 * SCALE, 1.f / 2.2f), 0.f, 0.999f);

    float4 t1 = read_imagef(temp, sampler, (int2)(x, y));
    float4 t2 = read_imagef(temp, sampler, (int2)(x + 1, y));
    if (tcnt > 0) {
        float k = 1.f / tcnt;
        p1 = mix(t1, p1, k);
        p2 = mix(t2, p2, k);
    }
    write_imagef(out, (int2)(x, y), p1);
    write_imagef(out, (int2)(x + 1, y), p2);
}
//
//kernel void unpack10bitAdjustLevels(global uchar *in, write_only image2d_t out, int4 bb, float4 wg, float bt) {
//    int x = get_global_id(0) * 2;
//    int y = get_global_id(1);
//    int stride = (IMAGE_WIDTH / 4) * 5;
//    int off = (y * stride + x) * 2;
//
//    int w1 = *(global int*)(in + off);
//    int w2 = *(global int*)(in + off + stride);
//
//    // 10 bit unpacking
//    int4 p1, p2;
//    p1.x = w1 & 0x3FF;
//    p1.y = (w1 >> 10) & 0x3FF;
//    p2.x = (w1 >> 20) & 0x3FF;
//    p2.y = convert_int(in[4]) << 2 | (w1 >> 30) & 0x3FF;
//    p1.z = w1 & 0x3FF;
//    p1.w = (w1 >> 10) & 0x3FF;
//    p2.z = (w1 >> 20) & 0x3FF;
//    p2.w = convert_int(in[4 + stride]) << 2 | (w1 >> 30) & 0x3FF;
//
//    // Level adjustments
//    float4 fbb = convert_float4(bb);
//    float4 fp1 = (convert_float4(p1) - fbb) * wg + (float4)bt;
//    float4 fp2 = (convert_float4(p1) - fbb) * wg + (float4)bt;
//
//    // Gamma encoding
//    fp1 = clamp(pow(fp1 * SCALE , 1.f / 2.2f), 0.f, 0.999f);
//    fp2 = clamp(pow(fp2 * SCALE, 1.f / 2.2f), 0.f, 0.999f);
//
//    write_imagef(out, (int2)(x, y), fp1);
//    write_imagef(out, (int2)(x + 1, y), fp2);
//}
