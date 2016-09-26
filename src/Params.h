
#ifndef RCCBPARAMS_H_
#define RCCBPARAMS_H_

#include <sstream>
#include <string>
#include <fstream>

#include "image.h"
#include "debug.h"

#define VERSION   "v3.0d"
#define U         0
#define V         1

#define DBG (dbg && x == X&&y==Y)
#define SQRT3 1.732050808
#define COMPATIBILITY_FACTOR (1.5/2*SQRT3)

#define PATH "kernels"

struct RCCB {
    int quiet;
    int dbg;

    int fd;
    int ch_en;
    double lm_abs;
    double lm_rel;
    double ln_str;
    double ch_bypass;
    double ch_abs;
    double ch_corner;
    double ch_rel;
//    double ci_abs;
//    double ci_rel;
    double cf_scale[4];
    int cf_sofs;
    int cf_mins;
    int cf_maxs;
//    unsigned cih_conf;
//    unsigned civ_conf;
//    unsigned civ_yfact;
//    unsigned civ_cfact;
//    unsigned cih_yfact;
//    unsigned cih_cfact;
//    unsigned cih_yamul;
//    unsigned civ_yamul;
//    unsigned cih_camul;
//    unsigned civ_camul;
//    unsigned civ_mixk;
//    unsigned cih_gbk;
//    unsigned civ_gbk;
    double saturation;
    double smooth_abs;
    double smooth_rel;
    double smooth_edge;
    double g1g2_center;
    double g1g2_corner;
    double zoom;
    int bayer;
    int bay_depth;
    int dem_en;
    unsigned int ww[4];
    unsigned int bw[4];
    double ba;
    double bam;
    double pf_enable;
    double wg[3];
    double wa[3];
    int bb[4];
    double bt;
    double pf_mix[2];
    double pf_off;
    double pf_luma[2];
    double desat_yd;
//    double desat_dscc;
//    double desat_bias[3];
//    double desat_detail_k;
    double yden_rel;
    double yden_abs;
    double ysh_th;
    double ysh_str;
    double ysh_max;
    double ptg_abs;
    double ptg_rel;
    double stg_abs;
    double stg_rel;
//    double stg_th;
    double stg_slope;
    double gamc_th;
    double gamc_slope;
    double che_rth;
    double che_ath;
    double che_str;
    double che_sat_th;
    double che_sat_str;
    double che_max;
    double omega_alpha;
    double bpc_alpha;
    double bpc_floor;

    double cc[9];

    int bt_defined;

    int width;
    int height;
    short *pixels;
    int type;
    int depth;
    int nFrames;
    int frameIndex;
    int nAggregate;

    RCCB() {
        setDefaults();

        fn_in = fn_out_dem = fn_out_bay = fn_out_lin = fn_out_srgb = fn_out_cc = NULL;
        nFrames = 0;
        pixels = NULL;
        frameIndex = 0;
        fd = 10;
        width = 0;
        height = 0;
        depth = 10;
        type = IT_CRBC;
        nAggregate = 0;
    }

    void setDefaults() {
        bt_defined = 0;
        ch_en = 3;
        lm_abs = 0.040;
        lm_rel = 1.00;
        ln_str = 0;
        ch_bypass = 0;
        ch_abs = 0.340;
        ch_corner = 0.;
        ch_rel = 0.00;
//        ci_abs = 0.02;
//        ci_rel = 0.12;
        cf_sofs = -1;
        cf_mins = 1;
        cf_maxs = 7;
//        cih_conf = 0x44aff;
//        civ_conf = 0x2249e;
//        civ_yfact = 0x48888;
//        civ_cfact = 0x48888;
//        cih_yfact = 0x48888;
//        cih_cfact = 0x28888;
//        cih_yamul = 0x8aaaa;
//        civ_yamul = 0x8aaaa;
//        cih_camul = 0xa6666;
//        civ_camul = 0xa6666;
//        civ_mixk = 0xf7310;
//        cih_gbk = 0;
//        civ_gbk = 0;
//        saturation = 1.0;
//        smooth_abs = 0.04;
//        smooth_rel = 0.2;
//        smooth_edge = 8.;
//        g1g2_center = 0;
//        g1g2_corner = 30 / 256.;
//        zoom = 1.0;
//        quiet = 0;
//        dbg = 0;
//        bayer = 0;
//        dem_en = 1;
//        ww[0] = -1;
//        ww[1] = -1;
//        ww[2] = -1;
//        ww[3] = -1;
//        bw[0] = -1;
//        bw[1] = -1;
//        bw[2] = -1;
//        bw[3] = -1;
//        ba = 0;
//        bam = 0.1;
//        pf_enable = 1.;
//        wg[0] = wg[1] = wg[2] = 1;
//        wa[0] = wa[1] = wa[2] = 0;
//        bb[0] = bb[1] = bb[2] = bb[3] = 0;
//        bt = 0.005;
//        pf_mix[0] = pf_mix[1] = 0.1;
//        pf_off = 0.05;
//        pf_luma[0] = 0.2990;
//        pf_luma[1] = 0.1140;
//        desat_yd = 0.05;
//        desat_dscc = -1;
//        desat_bias[0] = -0.5;
//        desat_bias[1] = 0.5;
//        desat_bias[2] = -0.5;
//        desat_detail_k = 6. / 16;
//        yden_rel = 0.03;
//        yden_abs = 0.10;
//        ysh_th = 0.5;
//        ysh_str = 1.5;
//        ysh_max = 0.5;
//        stg_th = 5 / 256.;
//        stg_slope = 64. / (60);
//        gamc_th = 0;
//        gamc_slope = 0;
//        che_rth = 2. / 16;
//        che_ath = 1. / 16;
//        che_str = 1.;
//        che_sat_th = 1. / 16;
//        che_sat_str = 2.;
//        che_max = 2.;
//        omega_alpha = -9; //off
        ch_en = 3;
        lm_abs = 0.040;
        lm_rel = 1.00;
        ln_str = 0;
        ch_bypass = 0;
        ch_abs = 0.340;
        ch_corner = 0.;
        ch_rel = 0.00;
        cf_sofs = -1;
        cf_mins = 1;
        cf_maxs = 63;
        cf_scale[0] = COMPATIBILITY_FACTOR;
        cf_scale[1] = 4./SQRT3;
        cf_scale[2] = 2.*COMPATIBILITY_FACTOR;
        cf_scale[3] = 4.;
        saturation = 1.0;
        smooth_abs = 0.04;
        smooth_rel = 0.2;
        smooth_edge = 8.;
        g1g2_center = 0;
        g1g2_corner = 30 / 256.;
        zoom = 1.0;
        quiet = 0;
        dbg = 0;
        bayer = 0;
        bay_depth = -1;
        dem_en = 1;
        ww[0] = ww[1] = ww[2] = ww[3] = 0;
        bw[0] = bw[1] = bw[2] = bw[3] = 0;
        ba = 0;
        bam = 0.1;
        pf_enable = 1.;
        wg[0] = wg[1] = wg[2] = 1;
        wa[0] = wa[1] = wa[2] = 0;
        bb[0] = bb[1] = bb[2] = bb[3] = 0;
        bt = 0.005;
        pf_mix[0] = pf_mix[1] = 0.1;
        pf_off = 0.05;
        pf_luma[0] = 0.2990;
        pf_luma[1] = 0.1140;
        yden_rel = 0.03;
        yden_abs = 0.10;
        ysh_th = 0.0005;
        ysh_str = 1.5;
        ysh_max = 0.5;
        ptg_abs = 0.001;
        ptg_rel = 0;
        stg_abs = 0.004;
        stg_rel = 0;
        stg_slope = 0.25;
        gamc_th = 0;
        gamc_slope = 0;
        che_rth = 2. / 16;
        che_ath = 1. / 16;
        che_str = 1.;
        che_sat_th = 1. / 16;
        che_sat_str = 2.;
        che_max = 2.;
        omega_alpha = -9; //off
        bpc_alpha = 0.5;
        bpc_floor = 0x20 / 1024.;

        memset(cc, 0, 9 * sizeof(double));
        cc[0] = cc[4] = cc[8] = 1;
    }

    char *fn_in,    // Input filename
            *fn_out_dem,    // bayer after demosaic
            *fn_out_bay,    // bayer after RCB to RGB
            *fn_out_lin,    // after denoise and sharpening
            *fn_out_srgb,   // sRGB image after gamma (-os)
            *fn_out_cc;     // RGB image after color correction (-oc)

    void parse_args(int argc, char **argv) {
        if (argc <= 1) {
            help(argv[0], "No arguments provided", 0);
            return;
        }

        for (int i = 1; i < argc; i++) {
            char *s = argv[i];
            if (s[0] == '-') {
                s++;
                if (strcmp(s, "i") == 0) {
                    if (fn_in)
                        help(argv[0], "Two input filenames not allowed", 0);
                    fn_in = argv[++i];
                } else if (strcmp(s, "v") == 0) {
                    trace("VERSION %s", VERSION);
                } else if (strcmp(s, "h") == 0) {
                    help(argv[0], NULL, 0);
                } else if (strcmp(s, "hh") == 0) {
                    help(argv[0], NULL, 1);
                } else if (strcmp(s, "q") == 0) {
                    quiet = 1;
                } else if (strcmp(s, "d") == 0) {
                    dbg = 1;
                } else if (strcmp(s, "z") == 0) {
                    zoom = atof(argv[++i]);
                } else if (strcmp(s, "fd") == 0) {
                    fd = atoi(argv[++i]);
                } else if (strcmp(s, "s") == 0) {
                    saturation = atof(argv[++i]);
                } else if (strcmp(s, "ce") == 0) {
                    ch_en = atoi(argv[++i]);
                } else if (strcmp(s, "cr") == 0) {
                    ch_rel = atof(argv[++i]);
                } else if (strcmp(s, "cb") == 0) {
                    ch_bypass = atof(argv[++i]);
                } else if (strcmp(s, "ca") == 0) {
                    ch_abs = atof(argv[++i]);
                } else if (strcmp(s, "cac") == 0) {
                    ch_corner = atof(argv[++i]);
                } else if (strcmp(s, "cs") == 0) {
                    cf_mins = atoi(argv[++i]);
                    cf_maxs = atoi(argv[++i]);
                    cf_sofs = atoi(argv[++i]);
                } else if (strcmp(s, "ck") == 0) {
                    cf_scale[2] = atof(argv[++i]);
                    cf_scale[0] = atof(argv[++i]);
                } else if (strcmp(s, "ckb") == 0) {
                    cf_scale[3] = atof(argv[++i]);
                    cf_scale[1] = atof(argv[++i]);
                } else if (strcmp(s, "lr") == 0) {
                    lm_rel = atof(argv[++i]);
                } else if (strcmp(s, "la") == 0) {
                    lm_abs = atof(argv[++i]);
                } else if (strcmp(s, "ln") == 0) {
                    ln_str = atof(argv[++i]);
                } else if (strcmp(s, "de") == 0) {
                    dem_en = atoi(argv[++i]);
                } else if (strcmp(s, "pl") == 0) {
                    pf_enable = atof(argv[++i]);
                } else if (strcmp(s, "po") == 0) {
                    pf_off = atof(argv[++i]);
                } else if (strcmp(s, "pm") == 0) {
                    pf_mix[0] = atof(argv[++i]);
                    pf_mix[1] = atof(argv[++i]);
                } else if (strcmp(s, "pw") == 0) {
                    pf_luma[0] = atof(argv[++i]);
                    pf_luma[1] = atof(argv[++i]);
                } else if (strcmp(s, "sa") == 0) {
                    smooth_abs = atof(argv[++i]);
                } else if (strcmp(s, "sr") == 0) {
                    smooth_rel = atof(argv[++i]);
                } else if (strcmp(s, "se") == 0) {
                    smooth_edge = atof(argv[++i]);
                } else if (strcmp(s, "g0") == 0) {
                    g1g2_center = atof(argv[++i]);
                } else if (strcmp(s, "gc") == 0) {
                    g1g2_corner = atof(argv[++i]);
                } else if (strcmp(s, "lna") == 0) {
                    yden_abs = atof(argv[++i]);
                } else if (strcmp(s, "lnr") == 0) {
                    yden_rel = atof(argv[++i]);
                } else if (strcmp(s, "lss") == 0) {
                    ysh_str = atof(argv[++i]);
                } else if (strcmp(s, "lst") == 0) {
                    ysh_th = atof(argv[++i]);
                } else if (strcmp(s, "lsm") == 0) {
                    ysh_max = atof(argv[++i]);
                } else if (strcmp(s, "B") == 0) {
                    bayer = 2;
                } else if (strcmp(s, "b") == 0) {
                    bayer = 1;
                } else if (strcmp(s, "bd") == 0) {
                    bay_depth = atoi(argv[++i]);
                } else if (strcmp(s, "cc") == 0) {
                    int j;
                    for (j = 0; j < 9; j++)
                        cc[j] = atof(argv[++i]);
                } else if (strcmp(s, "omega") == 0) {
                    if (strcmp(argv[++i], "off") == 0)
                        omega_alpha = -9.;
                    else
                        omega_alpha = atof(argv[i]);
                } else if (strcmp(s, "ww") == 0) {
                    ww[0] = atoi(argv[++i]);
                    ww[1] = atoi(argv[++i]);
                    ww[2] = atoi(argv[++i]);
                    ww[3] = atoi(argv[++i]);
                } else if (strcmp(s, "wm") == 0) {
                    wg[0] = atof(argv[++i]);
                    wg[1] = atof(argv[++i]);
                    wg[2] = atof(argv[++i]);
                } else if (strcmp(s, "wa") == 0) {
                    wa[0] = atof(argv[++i]);
                    wa[1] = atof(argv[++i]);
                    wa[2] = atof(argv[++i]);
                } else if (strcmp(s, "bw") == 0) {
                    bw[0] = atoi(argv[++i]);
                    bw[1] = atoi(argv[++i]);
                    bw[2] = atoi(argv[++i]);
                    bw[3] = atoi(argv[++i]);
                } else if (strcmp(s, "bm") == 0) {
                    bb[0] = atoi(argv[++i]);
                    bb[1] = atoi(argv[++i]);
                    bb[2] = atoi(argv[++i]);
                    bb[3] = atoi(argv[++i]);
                } else if (strcmp(s, "bt") == 0) {
                    bt = atof(argv[++i]);
                    bt_defined = 1;
                } else if (strcmp(s, "ba") == 0) {
                    ba = atof(argv[++i]);
                } else if (strcmp(s, "bam") == 0) {
                    bam = atof(argv[++i]);
                } else if (strcmp(s, "ptga") == 0) {
                    ptg_abs = atof(argv[++i]);
                } else if (strcmp(s, "ptgr") == 0) {
                    ptg_rel = atof(argv[++i]);
                } else if (strcmp(s, "stga") == 0) {
                    stg_abs = atof(argv[++i]);
                } else if (strcmp(s, "stgr") == 0) {
                    stg_rel = atof(argv[++i]);
                } else if (strcmp(s, "stgs") == 0) {
                    stg_slope = atof(argv[++i]);
                } else if (strcmp(s, "gct") == 0) {
                    gamc_th = atof(argv[++i]);
                } else if (strcmp(s, "gcs") == 0) {
                    gamc_slope = atof(argv[++i]);
                } else if (strcmp(s, "eda") == 0) {
                    che_ath = atof(argv[++i]);
                } else if (strcmp(s, "edr") == 0) {
                    che_rth = atof(argv[++i]);
                } else if (strcmp(s, "eds") == 0) {
                    che_str = atof(argv[++i]);
                } else if (strcmp(s, "edm") == 0) {
                    che_max = atof(argv[++i]);
                } else if (strcmp(s, "edst") == 0) {
                    che_sat_th = atof(argv[++i]);
                } else if (strcmp(s, "edss") == 0) {
                    che_sat_str = atof(argv[++i]);
                } else if (strcmp(s, "da") == 0) {
                    bpc_alpha = atof(argv[++i]);
                } else if (strcmp(s, "df") == 0) {
                    bpc_floor = atof(argv[++i]);
                } else if (strcmp(s, "od") == 0) {
                    fn_out_dem = argv[++i];
                } else if (strcmp(s, "ob") == 0) {
                    fn_out_bay = argv[++i];
                } else if (strcmp(s, "ol") == 0) {
                    fn_out_lin = argv[++i];
                } else if (strcmp(s, "oc") == 0) {
                    fn_out_cc = argv[++i];
                } else if (strcmp(s, "os") == 0) {
                    fn_out_srgb = argv[++i];
                } else if (strcmp(s, "pa") == 0 || strcmp(s, "dc") == 0 || strcmp(s, "ds") == 0
                        || strcmp(s, "dscc") == 0 || strcmp(s, "sgt") == 0 || strcmp(s, "sgs") == 0
                        || strcmp(s, "ia") == 0 || strcmp(s, "ir") == 0) {
                    i += 1;
                    trace("WARNING: '-%s' option is obsolete! Ignoring.", s);
                } else if (strcmp(s, "ics") == 0 || strcmp(s, "icc") == 0 || strcmp(s, "icy") == 0
                        || strcmp(s, "imc") == 0 || strcmp(s, "imy") == 0
                        || strcmp(s, "igb") == 0) {
                    i += 2;
                    trace("WARNING: '-%s' option is obsolete! Ignoring.", s);
                } else if (strcmp(s, "pf") == 0 || strcmp(s, "dsb") == 0) {
                    i += 3;
                    trace("WARNING: '-%s' option is obsolete! Ignoring.", s);
                } else if (strcmp(s, "width") == 0) {
                    width = atoi(argv[++i]);
                } else if (strcmp(s, "height") == 0) {
                    height = atoi(argv[++i]);
                } else if (strcmp(s, "pattern") == 0) {
                    char *pat = argv[++i];
                    if (strcmp(pat, "RCCB") == 0 || strcmp(pat, "rccb") == 0) {
                        type = IT_RCCB;
                    } else if (strcmp(pat, "CRBC") == 0 || strcmp(pat, "crbc") == 0) {
                        type = IT_CRBC;
                    }
                } else {
                    trace("Invalid parameter '%s'", argv[i]);
                }
                if (i > argc)
                    help(argv[0], "Missing required parameter", 0);
            } else {
                if (fn_in)
                    help(argv[0], "Two input filenames not allowed", 0);
                fn_in = argv[i];
            }
        }

        if (!fn_in)
            help(argv[0], "Input filename not specified", 0);

    }

    bool readParams() {
        if (!fn_in)
            return false;

        char *argv[256] = { };
        int argc = 0;
        std::string str, buf;
        std::ifstream file;
        char fname[256];

        sprintf(fname, "%s.params", fn_in);
        file.open(fname);

        if (!file) {
            return false;
        }

        char *token;
        while (std::getline(file, str)) {
            unsigned p1 = str.find_first_of("#");
            unsigned p2 = str.find_first_not_of("/t ");
            if (p1 != p2) {
                buf += str + " ";
            } else {
                //trace("skipping %s", str.c_str());
            }
        }

        file.close();

        token = strtok((char*) buf.c_str(), " \\\t");
        while (token != NULL) {
            argv[argc++] = token;
            token = strtok(NULL, " \\\t");
        }

        if (argc > 0) {
            setDefaults();
            parse_args(argc, argv);
            return true;
        }

        return false;
    }

    void help(const char *s, const char *err, int advanced) {

        quiet = 0; // make sure this gets traceed
        if (err) {
          trace("ERROR: %s\n", err);
          trace("Please type '%s -h' for help on usage.", s);
          return;
        }
        trace("ClarityPlus Processing Utility " VERSION " (C) Aptina 2011-2013");
        trace("USAGE: %s [-i] input_file.raw [options]",s);
        trace("");
        trace("OPTIONS:");
        trace("  -v                   trace version");
        trace("  -h                   traces basic help");
        trace("  -hh                  traces advanced help");
        trace("  -q                   enable quiet output");
        trace("  -d                   enable debug output");
        trace("  -b                   denotes input file is Bayer (use legacy Bayer procesing)");
        trace("  -B                   denotes input file is Bayer (use new Bayer procesing)");
        trace("  -s <num>             color saturation [default=%4.2f]", saturation);
        trace("  -fd <num>            denotes input file bit depth [default=<file default>]");
        trace("  -bd <num>            specifies output Bayer bit depth [default=<input>]");
        trace("  -cc <num>^9          specifies 9 coefficients of the CCM (use this for Bayer)");
        if (omega_alpha == -9) {
            trace("  -omega off|<num>     adjust omega-cap alpha threshold [default=off]");
        } else {
            trace("  -omega off|<num>     adjust omega-cap alpha threshold [default=%4.2f]", omega_alpha);
        }
        trace("  -lr <num>            adjust chroma filter relative luma strength [default=%5.3f]", lm_rel);
        trace("  -la <num>            adjust chroma filter absolute luma strength [default=%5.3f]", lm_abs);
        trace("  -ln <num>            adjust luma noise insert [default=%5.3f]", ln_str);
        trace("  -cb <num>            adjust chroma filter bypass [default=%5.3f]", ch_bypass);
        trace("  -cr <num>            adjust chroma filter relative chroma strength [default=%5.3f]", ch_rel);
        trace("  -ca <num>            adjust chroma filter absolute chroma strength [default=%5.3f]", ch_abs);
        trace("  -ce <0~7>            filter enable mask (1=BPC, 2=FIR, 4=yden) [default=%d]", ch_en);
        trace("  -de <0/1>            high-quality demosaicing enable [default=%d]", dem_en);
        trace("  -sa <num>            smooth luma absolute threshold [default=%5.3f]", smooth_abs);
        trace("  -sr <num>            smooth luma relative threshold [default=%5.3f]", smooth_rel);
        trace("  -se <num>            edge smooth luma relative threshold [default=%5.3f]", smooth_edge);
        trace("  -g0 <num>            g1g2 correction %% in center  [default=%5.3f]", g1g2_center);
        trace("  -gc <num>            g1g2 correction %% in corners [default=%5.3f]", g1g2_corner);
        trace("  -lna <num>           luma noise floor [default=%7.5f]", yden_abs);
        trace("  -lnr <num>           luma noise coefficient [default=%5.3f]", yden_rel);
        trace("  -lss <num>           luma sharpening strength [default=%5.2f]", ysh_str);
        trace("  -lst <num>           luma sharpening threshold [default=%4.2f]", ysh_th);
        trace("  -lsm <num>           luma sharpening max. overshoot [default=%4.2f]", ysh_max);
        trace("  -wm <rg gg bg>       specifies manual white balance gains [default={%4.2f,%4.2f,%4.2f}]",wg[0],wg[1],wg[2]);
        trace("  -wa <ra ga ba>       specifies manual white balance averages on raw image [default=off]");
        trace("  -ww <x0 y0 x1 y1>    coordinates of gray patch (for white balance) [default=disabled]");
        trace("  -bm <c0 c1 c2 c3>    manual black balance for four RCCB channels [default={%d,%d,%d,%d}]", bb[0],bb[1],bb[2],bb[3]);
        trace("  -bw <x0 y0 x1 y1>    coordinates of black patch (for black balance) [default=disabled]");
        trace("  -bt <target>         manual black offset same for all four RCCB channels, after WB [default=%4.2f]", bt);
        trace("  -ba <threshold>      auto black offset determined based on the %% of darkest pixels [default=%4.2f]", ba);
        trace("  -bam <threshold>     maximum auto black offset determined based on the %% of darkest pixels [default=%4.2f]", bam);
        trace("  -od <filename.raw>   generates a demosaiced RCB/RGB file (before denoise)");
        trace("  -ol <filename.bmp>   generates a RGB linear RGB file (after denoise, before PF&CCM)");
        trace("  -oc <filename.bmp>   generates a RGB linear RGB file (after PF&CCM)");
        trace("  -ob <filename.raw>   generates a Bayer RAW file (after PF&CCM)");
        trace("  -os <filename.bmp>   generates a sRGB file");
        trace("  -pl <0~1>            point denoise enable [default=%4.2f]", pf_enable);
        trace("  -pm <0~1> <0~1>      point filter mix coefficients [default=%4.2f %4.2f]", pf_mix[0], pf_mix[1]);
        trace("  -pw <0~1> <0~1>      point filter mix coefficients [default=%4.2f %4.2f]", pf_luma[0], pf_luma[1]);
        trace("  -po <0~1/9>          point filter low-light threshold [default=%4.2f]", pf_off);
        trace("  -ptga <num>          pull-to-gray absolute threshold [default=%2.4f]", stg_abs);
        trace("  -ptgr <num>          pull-to-gray relative threshold [default=%2.4f]", stg_rel);
        trace("  -stga <num>          snap-to-gray absolute threshold [default=%2.4f]", stg_abs);
        trace("  -stgr <num>          snap-to-gray relative threshold [default=%2.4f]", stg_rel);
        trace("  -stgs <num>          snap-to-gray slope [default=%2.4f]", stg_slope);
        trace("  -eda <num>           edge desat absolute threshold [default=%4.2f]", che_ath);
        trace("  -edr <num>           edge desat relative threshold [default=%4.2f]", che_rth);
        trace("  -eds <num>           edge desat slope [default=%4.2f]", che_str);
        trace("  -edm <num>           edge desat max [default=%4.2f]", che_max);
        trace("  -edst <num>          edge desat saturation threshold [default=%4.2f]", che_sat_th);
        trace("  -edss <num>          edge desat saturation slope [default=%4.2f]", che_sat_str);
        trace("  -da <num>            defect correction noise alpha [default=%4.2f]", bpc_alpha);
        trace("  -df <num>            defect correction noise floor [default=%5.3f]", bpc_floor);
        trace("");
        trace("EXAMPLE:");
        trace("%s rgbc_100lux.raw -cr 0.25 -ca 0.025 -w 1864 1834 1964 1934 -ob bayer_100lux.raw -os 100lux.bmp", s);

        if (advanced) {
            trace("\n\nADVANCED OPTIONS :");
            trace("  -cac <num>           adjust chroma filter absolute chroma add in corners [default=%5.3f]", ch_corner);
            trace("  -cs <min> <max> <o>  adjust chroma filter spreading [default=%d %d %d]", cf_mins, cf_maxs, cf_sofs);
            trace("  -ck <H> <V>          adjust chroma filter hor/ver scaling [default=%4.2f %4.2f]", cf_scale[2], cf_scale[0]);
            trace("  -ckb <H> <V>         adjust chroma filter big hor/ver scaling [default=%4.2f %4.2f]", cf_scale[3], cf_scale[1]);
            trace("  -gct <num>           gamut compression threshold [default=%4.2f]", gamc_th);
            trace("  -gcs <num>           gamut compression slope [default=%4.2f]", gamc_slope);
            trace("  -z <num>             image zoom [default=%4.2f]", zoom);
        }
    }
};

#endif /* RCCBPARAMS_H_ */
