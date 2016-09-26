//#define A inY = mad(inY, inY_d, inUV);
//#define B inY_d = mad(inY, inY_d, inUV);
//#define C inUV = mad(inY, inY_d, inUV);
/*
#define A inY = pow(inY_d, (PIX)2.2f); inY = powr(inY, (PIX)(1.f/2.2f));
#define B inY_d = powr(inY, (PIX)2.2f); inY_d = powr(inY_d, (PIX)(1.f/2.2f));
#define C inUV = powr(inY_d, (PIX)2.2f); inUV = powr(inUV, (PIX)(1.f/2.2f));
#define D A B C
#define E D   
#define PROCESS E E E E E E E E E E
    
//    PROCESS 
    half2 a, b;
    half c = native_powr(a, b);
    half a, b;
    half2 c = half_powr(a, (half)0.3f);
    c = a * b + d;
    c = mad(a, b, d);
    
    PIX outY = inY / 10.f;
    PIX outUV = inUV / 20.f;
    
*/
#undef  PTG_ABS  
#undef  PTG_REL
#undef  STG_AB
#undef  STG_REL
#undef  STG_SLOPE
#undef  CMIX_SLOPE
#undef  CMIX_FLOOR
#undef  LUMA_R       
#undef  LUMA_B       
#undef  CHE_SAT_TH   
#undef  CHE_SAT_STR  
#undef  CHE_MAX      
#undef  PF_ENABLE    
#undef  PF_FLOOR     
#undef  PF_MIX_SLOPE 
#undef  MAX_14BIT_INV 
#undef  MAX_14BIT 

/*
#define  PTG_ABS        (PIX)0.004944f
#define  PTG_REL        (PIX)0.004944f
#define  STG_ABS        (PIX)0.007996f
#define  STG_REL        (PIX)0.000000f
#define  STG_SLOPE      (PIX)0.250000f
#define  CMIX_SLOPE     (PIX)0.5f
#define  CMIX_FLOOR     (PIX)0.61f
#define  LUMA_R         (PIX)64.f
#define  LUMA_B         (PIX)64.f
#define  CHE_SAT_TH     (PIX)0.1f
#define  CHE_SAT_STR    (PIX)16.f
#define  CHE_MAX        (PIX)64.f
#define  PF_ENABLE      true
#define  PF_FLOOR       (PIX)16384.f
#define  PF_MIX_SLOPE   (PIX)0.1f
#define MAX_14BIT_INV   (PIX)(1.F / 255.f)
#define MAX_14BIT       (PIX)(255.f)
*/    
    PIX PTG_ABS      = (PIX)0.004944f;
    PIX PTG_REL      = (PIX)0.046875f;
    PIX STG_ABS      = (PIX)0.007996f;
    PIX STG_REL      = (PIX)0.000000f;
    PIX STG_SLOPE    = (PIX)0.250000f;
    PIX CMIX_SLOPE   = (PIX)0.500000f;
    PIX CMIX_FLOOR   = (PIX)0.000610f;
    PIX LUMA_R       = (PIX)0.273438f;
    PIX LUMA_B       = (PIX)0.117188f;
    PIX CHE_SAT_TH   = (PIX)16.000000f;
    PIX CHE_SAT_STR  = (PIX)2.000000f;
    PIX CHE_MAX      = (PIX)2.000000f;
    bool PF_ENABLE   = true;
    PIX PF_FLOOR     = (PIX)0.000977f;
    PIX PF_MIX_SLOPE = (PIX)25.000000f;
    
    PIX CC0 = (PIX)-0.593262f;
    PIX CC1 = (PIX)-0.330566f;
    PIX CC2 = (PIX)-1.148926f;
    PIX CC3 = (PIX)-0.558105f;
    PIX CC4 = (PIX)0.206543f;
    PIX CC5 = (PIX)-1.138184f;
    PIX CC6 = (PIX)0.000000f;
    PIX CC7 = (PIX)0.000000f;
    PIX CC8 = (PIX)0.000000f;   
    
    //-----------------------
    PIX pr = inUV;
    PIX py = inY;
    PIX pb = inUV;
    PIX yD = inY_d;


    // TODO: s-curve
//    PIX K1 = (PIX)0.5f;        
//    PIX K2 = (PIX)0.15f;        
//    PIX K3 = (PIX)0.52f;        
//    PIX K4 = (PIX)0.45f;        
//    pr = K1 + sqrt(pr * pr * K2 + K3);
//    pb = K1 + sqrt(pb * pb * K2 + K3);
//    py = K1 + sqrt(py * py * K2 + K3);
//    yD = K1 + sqrt(yD * yD * K2 + K3);
    
    // -------- ptg + stg --------
    PIX ptg_th = clamp((PTG_ABS + py * PTG_REL), (PIX)0.0f, (PIX)1.0f);
    PIX stg_th = clamp((STG_ABS + py * STG_REL), (PIX)0.0f, (PIX)1.0f);

    PIX pdr = fabs(pr - py);
    PIX pdb = fabs(pb - py);
    PIX pd  = pdr + pdb;

    PIX ratio_b = (pd > (PIX)0.001) ? pdb/pd : (PIX)1.0;
    ptg_th = (pd < stg_th) ? ptg_th : fmax((ptg_th - (pd-stg_th) * STG_SLOPE), (PIX)0.0f);
    PIX ptgtb = ptg_th * ratio_b;
    PIX ptgtr = ptg_th * ((PIX)1.0f - ratio_b);

    PIX ppr = pr;
    ppr = (pdr < ptgtr) ? py : ((pr > py) ? ppr - ptgtr : ppr + ptgtr);
    PIX ppb = pb;
    ppb = (pdb < ptgtb) ? py : ((pb > py) ? ppb - ptgtb : ppb + ptgtb);

    PIX rUp = clamp(ppr, (PIX)0.0f, (PIX)1.0f);
    PIX yUp = clamp(py,  (PIX)0.0f, (PIX)1.0f);
    PIX bUp = clamp(ppb, (PIX)0.0f, (PIX)1.0f);
        
    // -------- ymix --------
    PIX cdiff = yUp - yD;
    PIX cs = fmax(yUp, CMIX_FLOOR);
//    PIX fw = ((yD > (PIX)0.0f) && (CMIX_SLOPE < (PIX)3.9375f)) ? clamp(CMIX_SLOPE * fabs(cdiff) / yD, (PIX)0.0f, (PIX)1.0f) : (PIX)1.0;
    PIX fw = (yD > (PIX)0.f) ? clamp(CMIX_SLOPE * fabs(cdiff) / yD, (PIX)0.0f, (PIX)1.0f) : (PIX)1.0;
    PIX ffu= fmin(fw * fabs(cdiff) / cs, 1.99609375f);
    PIX ff = (cdiff > 0.0f) ? -ffu : ffu;

    PIX rf = clamp(((rUp - yUp) * ((PIX)1.0f + ff) + yD), (PIX)0.0f, (PIX)1.0f);
    PIX gf = yD;
    PIX bf = clamp(((bUp - yUp) * ((PIX)1.0f + ff) + yD), (PIX)0.0f, (PIX)1.0f);

    // -------- apply color correction matrix --------
    PIX rc = clamp(((1.f - CC1 - CC2) * rf + CC0 * gf + CC1 * bf), 0.0f, 1.0f);
    PIX gc = clamp(((1.f - CC3 - CC5) * gf + CC2 * rf + CC3 * bf), 0.0f, 1.0f);
    PIX bc = clamp(((1.f - CC6 - CC7) * bf + CC4 * rf + CC5 * gf), 0.0f, 1.0f);

    // -------- SFX --------
    PIX l_ccm  = gc + LUMA_R * (rc-gc) + LUMA_B * (bc-gc);
    PIX l_sfx = gf;
    PIX val;

    // -------- desaturation --------
    // -------- edge decc detection --------    
    // TODO:

    // -------- point filter, method S/3D --------
    PIX l_mix  = PF_ENABLE ? l_ccm : l_sfx;
    PIX l_safe = fmax(l_mix, PF_FLOOR);
    PIX ld  = l_sfx - l_mix;

    PIX fw_ = clamp((l_mix * PF_MIX_SLOPE), (PIX)0.0f, (PIX)1.0f);
    PIX ff_ = clamp(fw_ * ld / l_safe, (PIX)-1.0f, (PIX)1.0f);
        
    val = ld - l_mix * ff_;
    
    PIX rcc = clamp((rc * (1.0f + ff_) + val), (PIX)0.0f, (PIX)1.0f);
    PIX gcc = clamp((gc * (1.0f + ff_) + val), (PIX)0.0f, (PIX)1.0f);
    PIX bcc = clamp((bc * (1.0f + ff_) + val), (PIX)0.0f, (PIX)1.0f);
    // TODO: YUV conversion
    rcc = clamp( 0.256788f * rcc + 0.504129f * gcc + 0.097906f * bcc +  16.f, 0.f, 255.f);
    gcc = clamp( 0.439216f * rcc - 0.367788f * gcc - 0.071427f * bcc + 128.f, 0.f, 255.f);
    bcc = clamp(-0.148223f * rcc - 0.290993f * gcc + 0.439216f * bcc + 128.f, 0.f, 255.f);

    PIX outY = rcc;
    PIX outUV = gcc;  
  