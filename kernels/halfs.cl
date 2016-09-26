#ifdef cl_khr_fp16

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void img_h4(__read_only image2d_t in_yuv, __read_only image2d_t in_yuvd, __write_only image2d_t out_yuv) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int h = get_global_size(1);
    
    int2 posY = (int2)(x, y);
    int2 posUV = (int2)(x, h + y / 2);
    
    // READ 4 pixels of Y, YD, UV
    half4 inY = read_imageh(in_yuv, sampler, posY);
    half4 inY_d = read_imageh(in_yuvd, sampler, posY);
    half4 inUV = read_imageh(in_yuv, sampler, posUV);

#define PIX half4    
#include "pointMix.cl"
#undef PIX    
    
    // WRITE 4 pixels of Y, UV
    write_imageh(out_yuv, posY, outY);
    write_imageh(out_yuv, posUV, outUV);
}

__kernel void img_h8(__read_only image2d_t in_yuv, __read_only image2d_t in_yuvd, __write_only image2d_t out_yuv) {
    int x = get_global_id(0);
    int y = get_global_id(1) * 2;
    int h = get_global_size(1) * 2;

    int2 posY0 = (int2)(x, y);
    int2 posY1 = (int2)(x, y + 1);
    int2 posUV = (int2)(x, h + y / 2);
    
    // read 8 values of Y, 4 values of UV 
    half4 inY0 = read_imageh(in_yuv, sampler, posY0);
    half4 inY1 = read_imageh(in_yuv, sampler, posY1);
    half4 inY_d0 = read_imageh(in_yuvd, sampler, posY0);
    half4 inY_d1 = read_imageh(in_yuvd, sampler, posY1);
    half4 inUV0 = read_imageh(in_yuv, sampler, posUV);

    half8 inY = (half8)(inY0, inY1);
    half8 inUV = (half8)(inUV0, inUV0);
    half8 inY_d = (half8)(inY_d0, inY_d1);

#define PIX half8    
#include "pointMix.cl"
#undef PIX    
    
    // write 8 values of y, 4 values of uv
    write_imageh(out_yuv, posY0, outY.hi);
    write_imageh(out_yuv, posY1, outY.lo);
    write_imageh(out_yuv, posUV, outUV.hi);
}

__kernel void buf_h4(global uchar *in1, global uchar *in2, global uchar *out) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int w = get_global_size(0);
    int h = get_global_size(1);
    int offY = x + y * w; 
    int offUV = h * w + offY / 2; 
    
    // read 4 values of Y, YD, 4 values of UV
    half4 inY = convert_half4(vload4(offY, in1));
    half4 inY_d = convert_half4(vload4(offY, in2));
    half4 inUV = convert_half4(vload4(offUV, in1));
    

#define PIX half4    
#include "pointMix.cl"
#undef PIX    

    // write 4 values of Y and 4 values of UV    
    vstore4(convert_uchar4(outY), offY, out);
    vstore4(convert_uchar4(outUV), offUV, out);
}

__kernel void buf_h8(global uchar *in1, global uchar *in2, global uchar *out) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int w = get_global_size(0);
    int h = get_global_size(0);
    int offY = (x + y * w) * 2; 
    int offUV = h * w + offY / 2; 
    
    // read 8 values of Y, YD, 4 pixels of UV
    half8 inY = convert_half8(vload8(offY, in1));
    half8 inY_d = convert_half8(vload8(offY, in2));
    // TODO: fix it!
    half8 inUV = convert_half8(vload8(offUV, in1));
        
#define PIX half8    
#include "pointMix.cl"
#undef PIX    
    
    // write 8 values of Y and 4 values of UV    
    vstore8(convert_uchar8(outY), offY, out);
    vstore8(convert_uchar8(outUV), offUV, out);
}
#else
__kernel void buf_h4(global uchar *in1, global uchar *in2, global uchar *out) {}
__kernel void buf_h8(global uchar *in1, global uchar *in2, global uchar *out) {}
__kernel void img_h4(__read_only image2d_t in_yuv, __read_only image2d_t in_yuvd, __write_only image2d_t out_yuv) {}
__kernel void img_h8(__read_only image2d_t in_yuv, __read_only image2d_t in_yuvd, __write_only image2d_t out_yuv) {}
#endif