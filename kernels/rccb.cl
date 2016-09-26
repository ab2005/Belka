#include "rccb.h"

#define ALPHA 1.f
#define BETA  1.f
#define GAMMA 1.f
#define SCALE 255.f

//#define SRGB_SPACE

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

/*
 * A simple RCCB pipeline.
 */
__kernel void rccb(__read_only image2d_t in, __write_only image2d_t out) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    float fx = x + 0.5f;
    float fy = y + 0.5f;

#ifndef SRGB_SPACE
    float4 p1 = read_imagef(in, sampler, (float2)(fx - .5f, fy - .5f));
    float4 p2 = read_imagef(in, sampler, (float2)(fx     , fy - .5f));
    float4 p3 = read_imagef(in, sampler, (float2)(fx + .5f, fy - .5f));
    float4 p4 = read_imagef(in, sampler, (float2)(fx - .5f, fy     ));
    float4 p5 = read_imagef(in, sampler, (float2)(fx     , fy     ));
    float4 p6 = read_imagef(in, sampler, (float2)(fx + .5f, fy     ));
    float4 p7 = read_imagef(in, sampler, (float2)(fx - .5f, fy + .5f));
    float4 p8 = read_imagef(in, sampler, (float2)(fx     , fy + .5f));
    float4 p9 = read_imagef(in, sampler, (float2)(fx + .5f, fy + .5f));
#else
    float4 p1 = pow(read_imagef(in, sampler, (float2)(fx - .5f, fy - .5f)), 2.2f);
    float4 p2 = pow(read_imagef(in, sampler, (float2)(fx     , fy - .5f)), 2.2f);
    float4 p3 = pow(read_imagef(in, sampler, (float2)(fx + .5f, fy - .5f)), 2.2f);
    float4 p4 = pow(read_imagef(in, sampler, (float2)(fx - .5f, fy     )), 2.2f);
    float4 p5 = pow(read_imagef(in, sampler, (float2)(fx     , fy     )), 2.2f);
    float4 p6 = pow(read_imagef(in, sampler, (float2)(fx + .5f, fy     )), 2.2f);
    float4 p7 = pow(read_imagef(in, sampler, (float2)(fx - .5f, fy + .5f)), 2.2f);
    float4 p8 = pow(read_imagef(in, sampler, (float2)(fx     , fy + .5f)), 2.2f);
    float4 p9 = pow(read_imagef(in, sampler, (float2)(fx + .5f, fy + .5f)), 2.2f);
#endif
    float4 s28 = (p2 + p8);
    float4 s46 = (p4 + p6);
    float4 s2468 = s28 + s46;
    float r_dg = (p5.x - s2468.x / 4.f);
    float b_dg = (p5.w - s2468.w / 4.f);
    float cr_dr = (((p5.y * 6.f) + s28.y - s46.y * 2.f - p3.z * 4.f) / 8.f) * GAMMA;
    float cr_db = (((p5.z * 6.f) + s28.z - s46.z * 2.f - p7.y * 4.f) / 8.f) * GAMMA;
    float cb_dr = (((p5.z * 6.f) + s46.z - s28.z * 2.f - p7.y * 4.f) / 8.f) * GAMMA;
    float cb_db = (((p5.y * 6.f) + s46.y - s28.y * 2.f - p3.z * 4.f) / 8.f) * GAMMA;
    float4 rf = (float4)(p5.x, p6.x + cr_dr, p8.x + cb_dr, p9.x + b_dg * BETA);
    float4 gf = (float4)((p4.y + p2.z) / 2.f + r_dg * ALPHA, p5.y, p5.z, (p8.y + p6.z) / 2.f + b_dg * ALPHA);
    float4 bf = (float4)(p1.w + r_dg * BETA, p2.w + cr_db, p4.w + cb_db, p5.w);

    gf = gf * PF_MIX_C + rf * PF_MIX_R + bf * PF_MIX_B;

    // TODO: NR

    // CC
	float4 r = rf * (1.f - RCCB_cc1 - RCCB_cc2) + gf * RCCB_cc1 + bf * RCCB_cc2;
	float4 c = gf * (1.f - RCCB_cc3 - RCCB_cc5) + rf * RCCB_cc3 + bf * RCCB_cc5;
	float4 b = bf * (1.f - RCCB_cc6 - RCCB_cc7) + rf * RCCB_cc6 + gf * RCCB_cc7;

#ifdef SRGB_SPACE
    r = clamp(pow(r, 0.454545f), 0.f, 0.99f);
    c = clamp(pow(c, 0.454545f), 0.f, 0.99f);
    b = clamp(pow(b, 0.454545f), 0.f, 0.99f);
#endif

    x *= 2;
    y *= 2;
    write_imagef(out, (int2)(x, y), (float4)(r.x, c.x, b.x, 0));
    write_imagef(out, (int2)(x + 1, y), (float4)(r.y, c.y, b.y, 0));
    write_imagef(out, (int2)(x, y + 1), (float4)(r.z, c.z, b.z, 0));
    write_imagef(out, (int2)(x + 1, y + 1), (float4)(r.w, c.w, b.w, 0));
}

