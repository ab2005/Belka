#include "rccb.h"

#define  PTG_ABS   16384.f
#define  PTG_REL   256.f
#define  STG_ABS   16384.f
#define  STG_REL   256.f
#define  STG_SLOPE 64.f

#define  CMIX_SLOPE 32.f
#define  CMIX_FLOOR 16384.f

#define  LUMA_R       64.f
#define  LUMA_B       64.f
#define  CHE_SAT_TH   0.1f
#define  CHE_SAT_STR  16.f
#define  CHE_MAX      64.f
#define  PF_ENABLE    true
#define  PF_FLOOR     16384.f
#define  PF_MIX_SLOPE 0.1f
#define MAX_14BIT_INV (1.F / 255.f)
#define MAX_14BIT (255.f)

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void pixelMix_image_float4_nodesat_image(
                   __read_only image2d_t in_y,
                   __read_only image2d_t in_uv,
                   __read_only image2d_t in_yd,
                   __write_only image2d_t out)
{

#define PIX float4

    int x = get_global_id(0);
    int y = get_global_id(1);

    int2 posY = (int2)(x, y);
    int2 posUV = (int2)(x / 2, y);

    // read 4 pixels
    PIX inY = read_imagef(in_y, sampler, posY);
    PIX inUV = read_imagef(in_uv, sampler, posUV);
    PIX inY_d = read_imagef(in_yd, sampler, posY);

    //-----------------------
    PIX pr = inUV * MAX_14BIT_INV;
    PIX py = inY * MAX_14BIT_INV;
    PIX pb = inUV * MAX_14BIT_INV;
    PIX yD = inY_d * MAX_14BIT_INV;

    // -------- ptg + stg --------
    PIX ptg_th = clamp((PTG_ABS + py * PTG_REL), 0.0f, 1.0f);
    PIX stg_th = clamp((STG_ABS + py * STG_REL), 0.0f, 1.0f);

    PIX pdr = fabs(pr - py);
    PIX pdb = fabs(pb - py);
    PIX pd  = pdr + pdb;

    PIX ratio_b = (pd > (PIX)0.001) ? pdb/pd : (PIX)1.0;
    ptg_th = (pd < stg_th) ? ptg_th : fmax((ptg_th - (pd-stg_th) * STG_SLOPE), 0.0f);
    PIX ptgtb = ptg_th * ratio_b;
    PIX ptgtr = ptg_th * (1.0f - ratio_b);

    PIX ppr = pr;
    ppr = (pdr < ptgtr) ? py : ((pr > py) ? ppr - ptgtr : ppr + ptgtr);
    PIX ppb = pb;
    ppb = (pdb < ptgtb) ? py : ((pb > py) ? ppb - ptgtb : ppb + ptgtb);

    PIX rUp = clamp(ppr, 0.0f, 1.0f);
    PIX yUp = clamp(py,  0.0f, 1.0f);
    PIX bUp = clamp(ppb, 0.0f, 1.0f);

    // -------- ymix --------
    PIX cdiff = yUp - yD;
    PIX cs = fmax(yUp, CMIX_FLOOR);
    PIX fw = ((yD > (PIX)0.0f) && (CMIX_SLOPE < (PIX)3.9375f)) ? clamp(CMIX_SLOPE * fabs(cdiff) / yD, 0.0f, (PIX)1.0f) : (PIX)1.0;
    PIX ffu= fmin(fw * fabs(cdiff) / cs, 1.99609375f);
    PIX ff = (cdiff > 0.0f) ? -ffu : ffu;

    PIX rf = clamp(((rUp - yUp) * (1.0f + ff) + yD), 0.0f, 1.0f);
    PIX gf = yD;
    PIX bf = clamp(((bUp - yUp) * (1.0f + ff) + yD), 0.0f, 1.0f);

    // -------- apply color correction matrix --------
    PIX rc = clamp(((1.f - RCCB_cc1 - RCCB_cc2) * rf + RCCB_cc0 * gf + RCCB_cc1 * bf), 0.0f, 1.0f);
    PIX gc = clamp(((1.f - RCCB_cc3 - RCCB_cc5) * gf + RCCB_cc2 * rf + RCCB_cc3 * bf), 0.0f, 1.0f);
    PIX bc = clamp(((1.f - RCCB_cc6 - RCCB_cc7) * bf + RCCB_cc4 * rf + RCCB_cc5 * gf), 0.0f, 1.0f);

    // -------- edge decc detection --------
    PIX che = 0.0f;
    // -------- SFX --------
    PIX l_ccm  = gc + LUMA_R * (rc-gc) + LUMA_B * (bc-gc);
    PIX l_sfx = gf;
    PIX val;

    // -------- desaturation --------
//    PIX qr, qb;
//    rgb2qrqb(rf * MAX_14BIT, gf * MAX_14BIT, bf * MAX_14BIT, &qr, &qb); /////
//    PIX sat    = (fabs(qr) + fabs(qb)) * 0.5f;
//    PIX satsub = fmax(((sat - CHE_SAT_TH) * 0.25f), 0.0f) * CHE_SAT_STR;
//    PIX desat  = clamp(che - satsub, 0.0f, CHE_MAX);
//    desat *= MAX_14BIT_INV; /////
//    val = l_ccm * desat;
//    rc = ((1.0f - desat) * rc + val);
//    gc = ((1.0f - desat) * gc + val);
//    bc = ((1.0f - desat) * bc + val);

    // -------- point filter, method S/3D --------
    PIX l_mix  = PF_ENABLE ? l_ccm : l_sfx;
    PIX l_safe = fmax(l_mix, PF_FLOOR);
    PIX ld  = l_sfx - l_mix;
    PIX fw_ = clamp((l_mix * PF_MIX_SLOPE), 0.0f, 1.0f);
    PIX ff_ = clamp(fw_ * ld / l_safe, -1.0f, 1.0f);
    val = ld - l_mix * ff_;
    PIX rcc = clamp((rc * (1.0f + ff_) + val), 0.0f, 1.0f);
    PIX gcc = clamp((gc * (1.0f + ff_) + val), 0.0f, 1.0f);
    PIX bcc = clamp((bc * (1.0f + ff_) + val), 0.0f, 1.0f);

    //-----------------------

    PIX outY = gcc + bcc;
    PIX outUV = rcc;
    posUV = (int2)(x / 2, y + y / 2);
    // write 4 pixels
    write_imagef(out, posY, outY);
    write_imagef(out, posUV, outUV);
}

/*
 * Reads:  4 values from texture Y
 *         4 values from texture UV
 * Writes: 4 values to texture YUV
           4 valeus to texture YUV  
 */
__kernel void pixelMix_image_float4_nodesat(
                                __read_only image2d_t in_y,
                                __read_only image2d_t in_uv,
                                __read_only image2d_t in_yd,
                                __write_only image2d_t out_y,
                                __write_only image2d_t out_uv)
{
#define PIX float4
    int x = get_global_id(0);
    int y = get_global_id(1);

    int2 posY = (int2)(x, y);
    int2 posUV = (int2)(x / 2, y);

    // read 4 pixels
    PIX inY = read_imagef(in_y, sampler, posY);
    PIX inUV = read_imagef(in_uv, sampler, posUV);
    PIX inY_d = read_imagef(in_yd, sampler, posY);

    //-----------------------
    PIX pr = inUV * MAX_14BIT_INV;
    PIX py = inY * MAX_14BIT_INV;
    PIX pb = inUV * MAX_14BIT_INV;
    PIX yD = inY_d * MAX_14BIT_INV;

    // -------- ptg + stg --------
    PIX ptg_th = clamp((PTG_ABS + py * PTG_REL), 0.0f, 1.0f);
    PIX stg_th = clamp((STG_ABS + py * STG_REL), 0.0f, 1.0f);

    PIX pdr = fabs(pr - py);
    PIX pdb = fabs(pb - py);
    PIX pd  = pdr + pdb;

    PIX ratio_b = (pd > (PIX)0.001) ? pdb/pd : (PIX)1.0;
    ptg_th = (pd < stg_th) ? ptg_th : fmax((ptg_th - (pd-stg_th) * STG_SLOPE), 0.0f);
    PIX ptgtb = ptg_th * ratio_b;
    PIX ptgtr = ptg_th * (1.0f - ratio_b);

    PIX ppr = pr;
    ppr = (pdr < ptgtr) ? py : ((pr > py) ? ppr - ptgtr : ppr + ptgtr);
    PIX ppb = pb;
    ppb = (pdb < ptgtb) ? py : ((pb > py) ? ppb - ptgtb : ppb + ptgtb);

    PIX rUp = clamp(ppr, 0.0f, 1.0f);
    PIX yUp = clamp(py,  0.0f, 1.0f);
    PIX bUp = clamp(ppb, 0.0f, 1.0f);

    // -------- ymix --------
    PIX cdiff = yUp - yD;
    PIX cs = fmax(yUp, CMIX_FLOOR);
    PIX fw = ((yD > (PIX)0.0f) && (CMIX_SLOPE < (PIX)3.9375f)) ? clamp(CMIX_SLOPE * fabs(cdiff) / yD, 0.0f, (PIX)1.0f) : (PIX)1.0;
    PIX ffu= fmin(fw * fabs(cdiff) / cs, 1.99609375f);
    PIX ff = (cdiff > 0.0f) ? -ffu : ffu;

    PIX rf = clamp(((rUp - yUp) * (1.0f + ff) + yD), 0.0f, 1.0f);
    PIX gf = yD;
    PIX bf = clamp(((bUp - yUp) * (1.0f + ff) + yD), 0.0f, 1.0f);

    // -------- apply color correction matrix --------
    PIX rc = clamp(((1.f - RCCB_cc1 - RCCB_cc2) * rf + RCCB_cc0 * gf + RCCB_cc1 * bf), 0.0f, 1.0f);
    PIX gc = clamp(((1.f - RCCB_cc3 - RCCB_cc5) * gf + RCCB_cc2 * rf + RCCB_cc3 * bf), 0.0f, 1.0f);
    PIX bc = clamp(((1.f - RCCB_cc6 - RCCB_cc7) * bf + RCCB_cc4 * rf + RCCB_cc5 * gf), 0.0f, 1.0f);

    // -------- edge decc detection --------
    PIX che = 0.0f;
    // -------- SFX --------
    PIX l_ccm  = gc + LUMA_R * (rc-gc) + LUMA_B * (bc-gc);
    PIX l_sfx = gf;
    PIX val;

    // -------- desaturation --------
    //    PIX qr, qb;
    //    rgb2qrqb(rf * MAX_14BIT, gf * MAX_14BIT, bf * MAX_14BIT, &qr, &qb); /////
    //    PIX sat    = (fabs(qr) + fabs(qb)) * 0.5f;
    //    PIX satsub = fmax(((sat - CHE_SAT_TH) * 0.25f), 0.0f) * CHE_SAT_STR;
    //    PIX desat  = clamp(che - satsub, 0.0f, CHE_MAX);
    //    desat *= MAX_14BIT_INV; /////
    //    val = l_ccm * desat;
    //    rc = ((1.0f - desat) * rc + val);
    //    gc = ((1.0f - desat) * gc + val);
    //    bc = ((1.0f - desat) * bc + val);

    // -------- point filter, method S/3D --------
    PIX l_mix  = PF_ENABLE ? l_ccm : l_sfx;
    PIX l_safe = fmax(l_mix, PF_FLOOR);
    PIX ld  = l_sfx - l_mix;
    PIX fw_ = clamp((l_mix * PF_MIX_SLOPE), 0.0f, 1.0f);
    PIX ff_ = clamp(fw_ * ld / l_safe, -1.0f, 1.0f);
    val = ld - l_mix * ff_;
    PIX rcc = clamp((rc * (1.0f + ff_) + val), 0.0f, 1.0f);
    PIX gcc = clamp((gc * (1.0f + ff_) + val), 0.0f, 1.0f);
    PIX bcc = clamp((bc * (1.0f + ff_) + val), 0.0f, 1.0f);

    //-----------------------

    PIX outY = gcc;
    PIX outUV = rcc;
    // write 4 pixels of y
    write_imagef(out_y, posY, outY);
    write_imagef(out_y, posUV, outUV);
}
/*
__kernel void pixelMix_image_half4_nodesat_image(
                                                 __read_only image2d_t in_y,
                                                 __read_only image2d_t in_uv,
                                                 __read_only image2d_t in_yd,
                                                 __write_only image2d_t out)
{
#undef PIX
#define PIX half8
    int x = get_global_id(0) * 2;
    int y = get_global_id(1);

    int2 posY0 = (int2)(x, y);
    int2 posY1 = (int2)(x + 1, y);
    int2 posUV = (int2)(x / 2, y);

    // read 4 pixels
//    PIX inY = read_imageh(in_y, sampler, posY);
//    PIX inUV = read_imageh(in_uv, sampler, posUV);
//    PIX inY_d = read_imageh(in_yd, sampler, posY);

    PIX inY = (PIX)(read_imageh(in_y, sampler, posY0), read_imageh(in_y, sampler, posY1));
    PIX inY_d = (PIX)(read_imageh(in_yd, sampler, posY0), read_imageh(in_yd, sampler, posY1));
    half4 p = read_imageh(in_uv, sampler, posUV);
    PIX inUV = (PIX)(p, p);
    //-----------------------
    PIX pr = inUV * MAX_14BIT_INV;
    PIX py = inY * MAX_14BIT_INV;
    PIX pb = inUV * MAX_14BIT_INV;
    PIX yD = inY_d * MAX_14BIT_INV;

    // -------- ptg + stg --------
    PIX ptg_th = clamp((PTG_ABS + py * PTG_REL), 0.0f, 1.0f);
    PIX stg_th = clamp((STG_ABS + py * STG_REL), 0.0f, 1.0f);

    PIX pdr = fabs(pr - py);
    PIX pdb = fabs(pb - py);
    PIX pd  = pdr + pdb;

    PIX ratio_b = (pd > (PIX)0.001) ? pdb/pd : (PIX)1.0;
    ptg_th = (pd < stg_th) ? ptg_th : fmax((ptg_th - (pd-stg_th) * STG_SLOPE), 0.0f);
    PIX ptgtb = ptg_th * ratio_b;
    PIX ptgtr = ptg_th * (1.0f - ratio_b);

    PIX ppr = pr;
    ppr = (pdr < ptgtr) ? py : ((pr > py) ? ppr - ptgtr : ppr + ptgtr);
    PIX ppb = pb;
    ppb = (pdb < ptgtb) ? py : ((pb > py) ? ppb - ptgtb : ppb + ptgtb);

    PIX rUp = clamp(ppr, 0.0f, 1.0f);
    PIX yUp = clamp(py,  0.0f, 1.0f);
    PIX bUp = clamp(ppb, 0.0f, 1.0f);

    // -------- ymix --------
    PIX cdiff = yUp - yD;
    PIX cs = fmax(yUp, CMIX_FLOOR);
    PIX fw = ((yD > (PIX)0.0f) && (CMIX_SLOPE < (PIX)3.9375f)) ? clamp(CMIX_SLOPE * fabs(cdiff) / yD, 0.0f, (PIX)1.0f) : (PIX)1.0;
    PIX ffu= fmin(fw * fabs(cdiff) / cs, 1.99609375f);
    PIX ff = (cdiff > 0.0f) ? -ffu : ffu;

    PIX rf = clamp(((rUp - yUp) * (1.0f + ff) + yD), 0.0f, 1.0f);
    PIX gf = yD;
    PIX bf = clamp(((bUp - yUp) * (1.0f + ff) + yD), 0.0f, 1.0f);

    // -------- apply color correction matrix --------
    PIX rc = clamp(((1.f - RCCB_cc1 - RCCB_cc2) * rf + RCCB_cc0 * gf + RCCB_cc1 * bf), 0.0f, 1.0f);
    PIX gc = clamp(((1.f - RCCB_cc3 - RCCB_cc5) * gf + RCCB_cc2 * rf + RCCB_cc3 * bf), 0.0f, 1.0f);
    PIX bc = clamp(((1.f - RCCB_cc6 - RCCB_cc7) * bf + RCCB_cc4 * rf + RCCB_cc5 * gf), 0.0f, 1.0f);

    // -------- edge decc detection --------
    PIX che = 0.0f;
    // -------- SFX --------
    PIX l_ccm  = gc + LUMA_R * (rc-gc) + LUMA_B * (bc-gc);
    PIX l_sfx = gf;
    PIX val;

    // -------- desaturation --------
    //    PIX qr, qb;
    //    rgb2qrqb(rf * MAX_14BIT, gf * MAX_14BIT, bf * MAX_14BIT, &qr, &qb); /////
    //    PIX sat    = (fabs(qr) + fabs(qb)) * 0.5f;
    //    PIX satsub = fmax(((sat - CHE_SAT_TH) * 0.25f), 0.0f) * CHE_SAT_STR;
    //    PIX desat  = clamp(che - satsub, 0.0f, CHE_MAX);
    //    desat *= MAX_14BIT_INV; /////
    //    val = l_ccm * desat;
    //    rc = ((1.0f - desat) * rc + val);
    //    gc = ((1.0f - desat) * gc + val);
    //    bc = ((1.0f - desat) * bc + val);

    // -------- point filter, method S/3D --------
    PIX l_mix  = PF_ENABLE ? l_ccm : l_sfx;
    PIX l_safe = fmax(l_mix, PF_FLOOR);
    PIX ld  = l_sfx - l_mix;
    PIX fw_ = clamp((l_mix * PF_MIX_SLOPE), 0.0f, 1.0f);
    PIX ff_ = clamp(fw_ * ld / l_safe, -1.0f, 1.0f);
    val = ld - l_mix * ff_;
    PIX rcc = clamp((rc * (1.0f + ff_) + val), 0.0f, 1.0f);
    PIX gcc = clamp((gc * (1.0f + ff_) + val), 0.0f, 1.0f);
    PIX bcc = clamp((bc * (1.0f + ff_) + val), 0.0f, 1.0f);

    //-----------------------

    PIX outY = gcc;
    PIX outUV = rcc;
    posUV = (int2)(x / 2, y + y / 2);
    // write 4 pixels
    write_imageh(out, posY0, outY.lo);
    write_imageh(out, posY1, outY.hi);
    write_imageh(out, posUV, outUV.lo);

//    PIX outY = gcc;
//    PIX outUV = rcc;
//    // write 4 pixels
//    write_imageh(out_y, posY, outY);
//    write_imageh(out_uv, posUV, outUV);
//    //    write_imageh(out, posY, outY.hi);
//    //    write_imageh(out, posUV, outUV.hi);
//    //    write_imageh(out, posY, outY.lo);
}
*/
