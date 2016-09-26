#include "rccb.h"

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void img_f4(__read_only image2d_t in_yuv, __read_only image2d_t in_yuvd, __write_only image2d_t out_yuv) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int h = get_global_size(1);
    
    int2 posY = (int2)(x, y);
    int2 posUV = (int2)(x, h + y / 2);
    
    // READ 4 pixels of Y, YD, UV
    float4 inY = read_imagef(in_yuv, sampler, posY);
    float4 inY_d = read_imagef(in_yuvd, sampler, posY);
    float4 inUV = read_imagef(in_yuv, sampler, posUV);

#define PIX float4    
#include "pointMix.cl"
#undef PIX    
    
    // WRITE 4 pixels of Y, UV
    write_imagef(out_yuv, posY, outY);
    write_imagef(out_yuv, posUV, outUV);
}

__kernel void img_f8(__read_only image2d_t in_yuv, __read_only image2d_t in_yuvd, __write_only image2d_t out_yuv) {
    int x = get_global_id(0);
    int y = get_global_id(1) * 2;
    int h = get_global_size(1) * 2;

    int2 posY0 = (int2)(x, y);
    int2 posY1 = (int2)(x, y + 1);
    int2 posUV = (int2)(x, h + y / 2);
    
    // read 8 values of Y, 4 values of UV 
    float4 inY0 = read_imagef(in_yuv, sampler, posY0);
    float4 inY1 = read_imagef(in_yuv, sampler, posY1);
    float4 inY_d0 = read_imagef(in_yuvd, sampler, posY0);
    float4 inY_d1 = read_imagef(in_yuvd, sampler, posY1);
    float4 inUV0 = read_imagef(in_yuv, sampler, posUV);

    float8 inY = (float8)(inY0, inY1);
    float8 inUV = (float8)(inUV0, inUV0);
    float8 inY_d = (float8)(inY_d0, inY_d1);

#define PIX float8    
#include "pointMix.cl"
#undef PIX    
    
    // write 8 values of y, 4 values of uv
    write_imagef(out_yuv, posY0, outY.hi);
    write_imagef(out_yuv, posY1, outY.lo);
    write_imagef(out_yuv, posUV, outUV.hi);
}

__kernel void buf_f4(global uchar *in1, global uchar *in2, global uchar *out) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int w = get_global_size(0);
    int off = x + y * w; 
    
    // read 4 values of Y, YD, 4 values of UV
    float4 inY = convert_float4(vload4(off, in1));
    float4 inY_d = convert_float4(vload4(off, in2));
    float4 inUV = convert_float4(vload4(off / 2, in1));
    

#define PIX float4    
#include "pointMix.cl"
#undef PIX    

    // write 4 values of Y and 4 values of UV    
    vstore4(convert_uchar4(outY), off, out);
    vstore4(convert_uchar4(outUV), off / 2, out);
}

__kernel void buf_f8(global uchar *in1, global uchar *in2, global uchar *out) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int w = get_global_size(0);
    int off = x + y * w; 
    
    // read 8 values of Y, YD, 4 pixels of UV
    float8 inY = convert_float8(vload8(off, in1));
    float8 inY_d = convert_float8(vload8(off, in2));
    float8 inUV = convert_float8(vload8(off / 2, in1));
        
#define PIX float8    
#include "pointMix.cl"
#undef PIX    
    
    // write 8 values of Y and 4 values of UV    
    vstore8(convert_uchar8(outY), off, out);
    vstore8(convert_uchar8(outUV), off / 2, out);
}
