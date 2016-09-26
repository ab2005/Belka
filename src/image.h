/*
 * image.h
 *
 *  Created on: Mar 6, 2013
 *      Author: mikael
 */

#ifndef IMAGE_H_
#define IMAGE_H_

// Image types:
#define IT_RGB         0x00
#define IT_BAY         0x01
#define IT_RCCB        0x02
#define IT_YUV         0x03
#define IT_RCB         0x04
#define IT_CEF         0x05
#define IT_STREAM      0x06
#define IT_GRAY        0x07
#define IT_CRBC        0x08
#define IT_MASK        0xff

// image sub type flags
#define IST_LIN        0x000 // data is in linear space
#define IST_LS4_9      0x100 // data is in log s4.9 space
#define IST_LS3_9      0x200 // data is in log s3.9 space
#define IST_MASK       0xf00 // subtype mask
// image ordering
#define IST_ODDH       0x2000 // bayer / rccb ordering
#define IST_ODDV       0x4000 // bayer / rccb ordering
// image subsample
#define ISS_CSSH       0x10000  // chroma subsampling in dimension x
#define ISS_CSSV       0x20000  // chroma subsampling in dimension y
#define ISS_CALH       0x40000  // chroma alignment in dimension x
#define ISS_CALV       0x80000  // chroma alignment in dimension y
// image flip
#define IF_IMG_FLIPH   0x100000
#define IF_IMG_FLIPV   0x200000

#define R   0
#define G   1
#define B   2

#define MAX_NB         16

inline bool it_type_is_raw(unsigned t) {
    return (((t & IT_MASK) == IT_BAY) || ((t & IT_MASK) == IT_RCCB) || ((t & IT_MASK) == IT_STREAM)
            || ((t & IT_MASK) == IT_GRAY));
}

#pragma pack(1)

typedef struct {
// BITMAPFILEHEADER: BMP File Header Contents
    short bfType;       // The characters "BM"
    int bfSize;       // The size of the file in bytes
    short bfReserved1;  // Unused - must be zero
    short bfReserved2;  // Unused - must be zero
    int bfOffBits;    // Offset to start of Pixel Data
// BITMAPINFOHEADER: BMP Image Header Contents for Windows Format
    int biSize;         // Header Size - Must be at least 40
    int biWidth;        // Image width in pixels
    int biHeight;       // Image height in pixels
    short biPlanes;     // Must be 1
    short biBitCount;   // Bits per pixel - 1, 2, 4, 8, 16, 24, or 32
    int biCompression;  // Compression type (0 = uncompressed)
    int biSizeImage;    // Image Size - may be zero for uncompressed images
    int biXPelsPerMeter;    // Preferred resolution in pixels per meter
    int biYPelsPerMeter;    // Preferred resolution in pixels per meter
    int biClrUsed;      // Number Color Map entries that are actually used
    int biClrImportant; // Number of significant colors
} bmp_t;
#pragma pack()

struct Image {
    unsigned int width;
    unsigned int height;
    unsigned int depth;     // number of bits per sample
    unsigned int type;
    union {
        unsigned short **r;
        unsigned short **v;
        unsigned short **e;
    };
    union {
        unsigned short **g;
        unsigned short **y;
        unsigned short **c;
    };
    union {
        unsigned short **gry;  // explicit gray component
        unsigned short **bay;  // explicit bayer component
        unsigned short **b;
        unsigned short **u;
        unsigned short **f;
    };

    Image(const unsigned int width = 0, const unsigned int height = 0,
            const unsigned char depth = 8, const unsigned int type = IT_RGB, const short *data = 0);
    Image(const Image& ref);

    ~Image();

    void info(const char *text);

    //read image from file fn. If file name is "-.ext" then image is read from stdin
    //if file name is "-" then ccr format is used.
    static Image *read(char *fn);
    int write(char *fn);
};

inline int clip_nb(Image *img, int x) {
    // from oscar.c
    //return (x < 0) ? 0 : ((x > (1 << img->depth) - 1) ? (1 << img->depth) - 1 : x);
    return x & ((1 << img->depth) - 1); // from slog.c
}

inline int clip8(int x) {
    return (x < 0) ? 0 : ((x > 255) ? 255 : x);
}

inline int color(Image *bay, int x, int y) {
    if (bay->type & IST_ODDH)
        x++;
    if (bay->type & IST_ODDV)
        y++;
    if (y & 1)
        return (x & 1) ? G : B;
    else
        return (x & 1) ? R : G;
}

extern void (*error)(const char *str, ...);

int write_bmp(Image *img, char *fname);

Image *img_median5(Image *img);
Image *img_demosaic_bilinear(Image *bay);
Image* x_img_demosaic_bilinear(Image *bay);
Image *img_tobayer(Image *img);
Image *img_gamma(Image *img, double from, double to);
int compare(Image* img1, Image* img2);
Image* diff(Image* img1, Image* img2);
#endif /* IMAGE_H_ */
