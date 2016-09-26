/*
 * image.c
 *
 *  Created on: Mar 6, 2013
 *      Author: mikael
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include <cassert>
#include <cmath>
#include <ctype.h>
#include "image.h"
#include "debug.h"

struct ByteBuffer {
    unsigned char *data;
    int len;

    ByteBuffer(int size) {
        data = new unsigned char[size];
        len = size;
    }
    ~ByteBuffer() {
        delete[] data;
    }

    static ByteBuffer *read(char *fn);
};

struct RawType {
    int width;
    int height;
    int type;
    int depth;
    int shift;
};

char err_str[512];

void _error(const char *str, ...) {
    va_list args;
    va_start(args, str);
    vsprintf(err_str, str, args);
    va_end(args);
}
void (*error)(const char *str, ...) = _error;

static int file_length(FILE *f) {
    int pos;
    int end;

    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, pos, SEEK_SET);
    return end;
}

/* common read */
ByteBuffer *ByteBuffer::read(char *fn) {
    FILE *f;
    int fsize;
    ByteBuffer *bytebuffer;

    if (fn == NULL) { //must read from stdin
        char *buf;
        int cnt;
        int bufsize, bufempty;

        bufsize = bufempty = 15 * 1024 * 1024;
        buf = new char[bufsize];
        if (!buf) {
            error("Can not allocate buffer for stdin read\n");
            return NULL;
        }

        fsize = 0;
        while (1) {
            cnt = fread(buf + fsize, 1, bufempty, stdin);
            fsize += cnt;
            if (cnt == bufempty) { //must increment
                int newsize = bufsize * 2;
                char *tmp = (char*) realloc(buf, newsize);
                if (!tmp) {
                    delete[] buf;
                    error("Can not allocate buffer for stdin read\n");
                    return NULL;
                }

                buf = tmp;
                bufempty = newsize - bufsize;
                bufsize = newsize;
            } else { //read whole data
                break;
            }
        }

        bytebuffer = new ByteBuffer(fsize);
        if (!bytebuffer) {
            error("Can not allocate bytebuffer\n");
            delete[] buf;
            return NULL;
        }
        memcpy(bytebuffer->data, buf, fsize);
        delete[] buf;

    } else { //read from file
        if ((f = fopen(fn, "rb")) == NULL) {
            error("Can not open %s for reading.", fn);
            return NULL;
        }
        fsize = file_length(f);
        bytebuffer = new ByteBuffer(fsize);
        if (!bytebuffer) {
            error("Can not allocate bytebuffer\n");
            fclose(f);
            return NULL;
        }
        if (fread(bytebuffer->data, fsize, 1, f) != 1) {
            error("read error");
            return NULL;
        }
        fclose(f);
    }
    return bytebuffer;
}

static int bayer_get(Image *bay, int x, int y) {
    if (x < 0)
        x &= 1;
    if (y < 0)
        y &= 1;
    if (x >= (int) bay->width)
        x = bay->width - 2 + ((x ^ bay->width) & 1);
    if (y >= (int) bay->height)
        y = bay->height - 2 + ((y ^ bay->height) & 1);
    //assert(x >= 0 && y >= 0 && x < bay->width && y < bay->height);
    return bay->b[y][x];
}

const char *default_inf = "1843200,1280,720, 1, 10\n"
        "6356992,2048,1544,1, 10\n" // mi9T013: 8 rows border
        "9902208,2568,1928,1, 12\n";// mt9p012: 4 pixels border each side

char *read_inf(char *fn) {
    FILE *f;
    //printf("read %s\n", fn);
    if ((f = fopen(fn, "rb")) == NULL) {
        return strdup("");
    }

    int l = file_length(f);
    char *n = new char[l + 1];
    fread(n, l, 1, f);
    n[l] = 0;
    fclose(f);
    //printf("'%s'\n %d\n", n, l);
    return n;
}

typedef struct {
    const char *name;
    unsigned type;
} img_type_t;

const img_type_t img_types[] = { { "RGB", IT_RGB }, { "RCB", IT_RCB }, { "YUV444", IT_YUV }, {
        "GRAY", IT_GRAY }, { "GRBG", IT_BAY }, { "RGGB", IT_BAY | IST_ODDH }, { "BGGR", IT_BAY
        | IST_ODDV }, { "GBRG", IT_BAY | IST_ODDH | IST_ODDV }, { "CRBC", IT_RCCB }, { "RCCB",
        IT_RCCB | IST_ODDH }, { "BCCR", IT_RCCB | IST_ODDV }, { "CBRC", IT_RCCB | IST_ODDH
        | IST_ODDV }, { NULL, IT_BAY } };

const img_type_t img_orientation[] = { { "0", 0 }, { "0FH", IF_IMG_FLIPH }, { "0FV", IF_IMG_FLIPV },
        { "180", IF_IMG_FLIPH | IF_IMG_FLIPV }, { "GRBG", IT_BAY }, { NULL, 0 } };

#define MAX_VAR 16
void set_var(int& nvar, char *var[MAX_VAR][2], const char *v, const char *t) {
    int i;
    //printf("'%s' = '%s'\n",v,t);
    for (i = 0; i < nvar; i++)
        if (strcmp(var[i][0], v) == 0)
            break;

    if (i == nvar) {
        nvar++;
        assert(nvar < MAX_VAR);
    }
    var[i][0] = (char*) v;
    var[i][1] = (char*) t;
}

int parse_inf(char *inf, int l, RawType *t) {
    int nvar = 0;
    char *var[MAX_VAR][2];

    int i;
    char size_str[10];
    sprintf(size_str, "%d", l);
    set_var(nvar, var, "size", size_str);
    set_var(nvar, var, "width", "0");
    set_var(nvar, var, "height", "0");
    set_var(nvar, var, "type", "GRBG");
    set_var(nvar, var, "depth", "0");
    set_var(nvar, var, "shift", "0");
    set_var(nvar, var, "rotate", "0");
    set_var(nvar, var, "version", "0");

    while (*inf) {
        while (isspace(*inf))
            inf++;
        if (!*inf)
            break;
        //printf("1[%s]\n", inf);

        char *t1 = inf;
        while (isalnum(*inf))
            inf++;
        char *t1e = inf;

        while (isspace(*inf))
            inf++;
        if (!*inf)
            return 1;

        //printf("2[%s]\n", inf);
        int op = 0;
        if (inf[0] == '=' && inf[1] == '=')
            op = 2, inf += 2;
        else if (inf[0] == '=')
            op = 1, inf += 1;
        else
            return 2;
        //printf("o%d[%s]\n", op, inf);
        //printf("3e[%s]\n", t1e);
        t1e[0] = 0;
        //printf("3[%s]\n", inf);

        while (isspace(*inf))
            inf++;
        if (!*inf)
            return 1;
        //printf("4[%s]\n", inf);
        char *t2 = inf;
        while (isalnum(*inf))
            inf++;
        char *t2e = inf;
        while (isspace(*inf))
            inf++;
        if (!*inf)
            return 1;
        //printf("5[%s]\n", inf);
        char tmp = t2e[0];
        t2e[0] = 0;
        //printf("'%s' %d '%s' <%s>\n", t1,op,t2, inf);

        /* execute instruction */
        const char *find = ",;";
        if (op == 1)
            set_var(nvar, var, t1, t2);
        else if (op == 2) {
            int i;
            for (i = 0; i < nvar; i++)
                if (strcmp(var[i][0], t1) == 0)
                    break;
            if (i == nvar)
                return 3;
            if (strcmp(var[i][1], t2) != 0)
                find = ";";
        } else
            assert(0);

        t2e[0] = tmp;
        while (strchr(find, *inf) == NULL) {
            if (!*inf)
                return 1;
            inf++;
        }
        t2e[0] = 0;
        inf++;
        //printf("6[%s]\n", inf);
    }

    //for (i = 0; i < nvar; i++)
    //  printf("'%s' = '%s'\n", var[i][0], var[i][1]);

    if (strcmp(var[7][1], "2") != 0)
        return 6;
    t->width = atoi(var[1][1]);
    t->height = atoi(var[2][1]);
    t->depth = atoi(var[4][1]);
    t->shift = atoi(var[5][1]);
    for (i = 0; img_types[i].name; i++) {
        if (strcmp(img_types[i].name, var[3][1]) == 0)
            break;
    }
    if (img_types[i].name == NULL)
        return 4;
    t->type = img_types[i].type;
    for (i = 0; img_orientation[i].name; i++) {
        if (strcmp(img_orientation[i].name, var[6][1]) == 0)
            break;
    }
    if (img_orientation[i].name == NULL)
        return 5;
    t->type |= img_orientation[i].type;
    return 0;
}

int check_inf(char *inf, int l, RawType *t) {
    int l2, w2, h2, d2, t2, s2;
    t->type = 0;
    t->depth = 10;
    t->shift = 0;
    t->width = t->height = 0;
    while (*inf) {
        int n = sscanf(inf, "%d,%d,%d,%d,%d,%d", &l2, &w2, &h2, &t2, &d2, &s2);
        if (l2 == l) {
            if (--n > 0)
                t->width = w2;
            if (--n > 0)
                t->height = h2;
            if (--n > 0)
                t->type = t2;
            if (--n > 0)
                t->depth = d2;
            if (--n > 0)
                t->shift = s2;
            return 0;
        }
        while (*inf && *inf != '\n')
            inf++;
        while (isspace(*inf))
            inf++;
    }

    int s = l / 2;
    if (l % 2)
        goto no_border;
    l /= 2;

    // assume 8 pixel border
    if (l % 2)
        goto one_byte;
    l = 3 * l + 4;
    if (l < 0)
        goto one_byte;

    {
        int r = (int) sqrt((double) l);
        if (r * r != l)
            goto one_byte;
        r = r - 14;
        if (r % 6)
            goto one_byte;
        t->height = 3 * r / 6 + 8;
        t->width = 4 * r / 6 + 8;
        if (t->height * t->width != s)
            t->width = -1;
    }

    one_byte: if (t->width <= 0) {
        l = s * 2;
        // assume 8 pixel border
        if (l % 2)
            goto no_border;
        l = 3 * l + 4;
        if (l < 0)
            goto no_border;
        int r = (int) sqrt((double) l);
        if (r * r != l)
            goto no_border;
        r = r - 14;
        if (r % 6)
            goto no_border;
        t->height = 3 * r / 6 + 8;
        t->width = 4 * r / 6 + 8;
        if (t->height * t->width != s * 2)
            t->width = -1;
        t->depth = 8;
    }

    no_border: if (t->width <= 0) {
        if ((s % 12) == 0) {
            l = s / 12;
            int r = (int) sqrt((double) l);
            if (r * r != l) {
                error("Unknown file size");
                return 1;
            }
            t->width = r * 4;
            t->height = r * 3;
        } else {
            error("Unknown file size");
            return 1;
        }
    }
    return 0;
}

unsigned short **alloc2D(int columns, int rows) {
    int i;
    unsigned short **array2D;
    if ((array2D = (unsigned short**) calloc(rows, sizeof(unsigned short*))) == NULL) {
        error("No memory for 2D array");
        return NULL;
    }
    if ((array2D[0] = (unsigned short*) calloc(columns * rows, sizeof(unsigned short))) == NULL) {
        error("No memory for 2D array");
        return NULL;
    }

    for (i = 1; i < rows; i++)
        array2D[i] = array2D[i - 1] + columns;

    return array2D;
}

int free2D(unsigned short **array2D) {
    if (array2D) {
        if (array2D[0])
            delete[] array2D[0];
        else {
            error("free_mem2D: trying to free unused memory");
            return 1;
        }
        delete array2D;
    } else {
        error("free_mem2D: trying to free unused memory");
        return 1;
    }
    return 0;
}

/* destructive reallocation of image memory */
int img_realloc(Image *img) {
    if (img == NULL) {
        error("img_realloc: trying to free unused memory");
        return 1;
    }
    free2D(img->r);
    free2D(img->g);
    free2D(img->b);

    if (!it_type_is_raw(img->type)) {
        if ((img->r = alloc2D(img->width, img->height)) == NULL)
            return 1;
        if ((img->g = alloc2D(img->width, img->height)) == NULL)
            return 1;
    }
    if ((img->b = alloc2D(img->width, img->height)) == NULL)
        return 1;
    return 0;
}

Image *read_raw(const char *fn, ByteBuffer *buff) {
    Image *img;
    unsigned int x, y;
    RawType t;
    unsigned char *data = buff->data;

    if (fn == NULL)
        fn = "stdin";

    char tmp[1024];

    sprintf(tmp, "%s.inf", fn);
    char *inf1 = read_inf(tmp);
    {
        int i, last = -1;
        for (i = 0; tmp[i]; i++)
            if (tmp[i] == '/' || tmp[i] == '\\')
                last = i;
        strcpy(&tmp[last + 1], "index.inf");
    }
    char *inf2 = read_inf(tmp);
    if ((inf1[0] == '\0' || strncmp(inf1, "version", 7) == 0)
            && (inf2[0] == '\0' || strncmp(inf2, "version", 7) == 0)) {
        int l1 = strlen(inf1);
        int l2 = strlen(inf2);
        char *inf = new char[l1 + l2 + 1];
        sprintf(inf, "%s%s", inf1, inf2);
        delete[] inf1;
        delete[] inf2;
        //printf("NEW INF '%s'\n", inf);
        if (parse_inf(inf, buff->len, &t))
            return NULL;
    } else {
        int l1 = strlen(inf1);
        int l2 = strlen(inf2);
        int l3 = strlen(default_inf);
        char *inf = new char[l1 + l2 + l3 + 1];
        sprintf(inf, "%s%s%s", inf1, inf2, default_inf);
        delete[] inf1;
        delete[] inf2;
        //printf("OLD INF '%s'\n", inf);

        if (check_inf(inf, buff->len, &t))
            return NULL;
    }

    if (t.depth > 16 || t.depth < 1) {
        fprintf(stderr, "Invalid depth %d\n", t.depth);
        return NULL;
    }
    //printf("%d x %d x %d sh %d\n", t.width, t.height, t.depth, t.shift);
    img = new Image(t.width, t.height, t.depth, t.type);
    //printf("%d (%dx%d)\n", buff->len, img->width, img->height);

    for (y = 0; y < img->height; y++)
        for (x = 0; x < img->width; x++) {
            int xx = (img->type & IF_IMG_FLIPH) ? (img->width - 1 - x) : x;
            int yy = (img->type & IF_IMG_FLIPV) ? (img->height - 1 - y) : y;
            if (t.depth > 8) {
                unsigned short u;
                u = *data++;
                u |= ((unsigned int) (*data++)) << 8;
#ifdef MSB_LSB_FLIP
                img->b[yy][xx] = ((u >> 8) & 0xff) | ((u & 0xff) << 8);
#else
                img->b[yy][xx] = u >> t.shift;
#endif

            } else {
                img->b[yy][xx] = (*data++) >> t.shift;
            }
        }

    return img;
}

Image *Image::read(char *fn) {
    ByteBuffer *bytebuffer;
    char *ext = strrchr(fn, '.');
    Image *img;
    if (ext == NULL) {
        if (strcmp(fn, "-") == 0) { //read from stdin
            fn = NULL;
        } else {
            error("Cannot determine file extension");
            return NULL;
        }
    } else {
        if (fn[0] == '-' && fn[1] == '.') { //read from stdin
            fn = NULL;
        }
    }

    { //read to bytebuff
        bytebuffer = ByteBuffer::read(fn);
        if (bytebuffer == NULL)
            return NULL;
        if (strcasecmp(ext, ".raw") == 0) {
            img = read_raw(fn, bytebuffer);
        } else {
            error("Unknown file extension '%s'", ext);
            img = NULL;
        }
        delete bytebuffer;
    }

    return img;
}

int write_bmp(Image *img, char *fn) {
    FILE *fo;
    unsigned char *bytebuffer;
    unsigned int x, w, h;
    int y, pad_line;
    bmp_t bmp;

    trace("write_bmp\n");

    if (it_type_is_raw(img->type)) {
        trace("Bayer should be demosaiced first");
        return 1;
    }
    if (img->depth != 8) {
        trace("image depth should be 8");
        return 1;
    }

    /* Opens the output file */
    if (fn) { //write to file
        if ((fo = fopen(fn, "wb+")) == NULL) {
            trace("Can't open %s\n", fn);
            return 1;
        }
    } else { //write to stdout
        fo = stdout;
    }

    w = img->width;
    h = img->height;

    pad_line = (4 - 3 * w) % 4;
    if (pad_line < 0)
        pad_line = 4 + pad_line;
    assert(pad_line >= 0);
    assert(pad_line <= 3);
    bmp.bfType = 19778;
    bmp.bfSize = 54 + (3 * w + pad_line) * h;
    bmp.bfReserved1 = 0;
    bmp.bfReserved2 = 0;
    bmp.bfOffBits = 54;
    bmp.biSize = 40;
    bmp.biWidth = w;
    bmp.biHeight = h;
    bmp.biPlanes = 1;
    bmp.biBitCount = 24;
    bmp.biCompression = 0;
    bmp.biSizeImage = 0;
    bmp.biXPelsPerMeter = 0;
    bmp.biYPelsPerMeter = 0;
    bmp.biClrUsed = 0;
    bmp.biClrImportant = 0;

    /* write header */
    if (fwrite(&bmp.bfType, 2, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.bfSize, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.bfReserved1, 2, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.bfReserved2, 2, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.bfOffBits, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biSize, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biWidth, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biHeight, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biPlanes, 2, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biBitCount, 2, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biCompression, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biSizeImage, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biXPelsPerMeter, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biYPelsPerMeter, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biClrUsed, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }
    if (fwrite(&bmp.biClrImportant, 4, 1, fo) != 1) {
        trace("write error");
        return 1;
    }

    bytebuffer = new unsigned char[(3 * w + pad_line) * h];
    int pp = 0; // pixel position inside bytebuffer
    for (y = img->height - 1; y >= 0; y--) {
        for (x = 0; x < img->width; x++) {
            bytebuffer[pp + 0] = clip8(img->b[y][x]);
            bytebuffer[pp + 1] = clip8(img->g[y][x]);
            bytebuffer[pp + 2] = clip8(img->r[y][x]);
            pp += 3;
        }
        pp += pad_line;
    }
    if (fwrite(bytebuffer, (3 * w + pad_line) * h, 1, fo) != 1) {
        trace("write error");
        return 1;
    }

    if (fn)
        fclose(fo);

    delete[] bytebuffer;
    return 0;
}

int exp_(short num, short e, short n, short d) {
    int sign = e + n + 1;
    double t = (double) ((num & ((1 << sign) - 1)) | ~((num & (1 << (sign - 1))) - 1));
    int o;
    t = t / ((double) (1 << n));

    t = pow(2, t);
    o = (int) t;
    o = o < 0 ? 0 : o;
    return o > ((1 << d) - 1) ? ((1 << d) - 1) : o;
}

// chroma upsampling in the X dimension
void imgyuv_cus_x(Image *img, int cal) {
    unsigned int x, y;
    unsigned int l, r;
    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
            if ((x ^ cal) & 0x1) {
                l = (x == 0) ? x + 1 : x - 1;  // left
                r = (x == img->width - 1) ? x - 1 : x + 1;  // right
                img->u[y][x] = (img->u[y][l] + img->u[y][r]) / 2;
                img->v[y][x] = (img->v[y][l] + img->v[y][r]) / 2;
            }
        }
    }
    img->type &= ~ISS_CSSH;
    img->type &= ~ISS_CALH;
}

// chroma upsampling in the Y dimension
void imgyuv_cus_y(Image *img, int cal) {
    unsigned int x, y;
    unsigned int u, d;
    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
            if ((y ^ cal) & 0x1) {
                u = (y == 0) ? y + 1 : y - 1;  // up
                d = (y == img->height - 1) ? y - 1 : y + 1;  // down
                img->u[y][x] = (img->u[u][x] + img->u[d][x]) / 2;
                img->v[y][x] = (img->v[u][x] + img->v[d][x]) / 2;
            }
        }
    }
    img->type &= ~ISS_CSSV;
    img->type &= ~ISS_CALV;
}

// chroma un-binning in the X dimension
void imgyuv_cub_x(Image *img, int cal) {
    unsigned int x, y;
    unsigned int pos;
    unsigned int u_l, u_r = 0;
    unsigned int v_l, v_r = 0;
    for (y = 0; y < img->height; y++) {
        pos = cal;
        // temp variables
        u_l = img->u[y][pos];
        v_l = img->v[y][pos];
        // write left border
        img->u[y][0] = u_l;
        img->v[y][0] = v_l;
        for (x = 0; x < img->width - 2; x += 2) {
            pos = pos + 2;
            // prepare right point
            u_r = img->u[y][pos];
            v_r = img->v[y][pos];
            // write un-binned points
            img->u[y][x + 1] = (3 * u_l + 1 * u_r) / 4;
            img->v[y][x + 1] = (3 * v_l + 1 * v_r) / 4;
            img->u[y][x + 2] = (1 * u_l + 3 * u_r) / 4;
            img->v[y][x + 2] = (1 * v_l + 3 * v_r) / 4;
            // copy right point into left point
            u_l = u_r;
            v_l = v_r;
        }
        // write right border
        img->u[y][img->width - 1] = u_r;
        img->v[y][img->width - 1] = v_r;
    }
    img->type &= ~ISS_CSSH;
    img->type &= ~ISS_CALH;
}

// chroma un-binning in the Y dimension
void imgyuv_cub_y(Image *img, int cal) {
    unsigned int x, y;
    unsigned int pos;
    unsigned int u_u[img->width], u_d[img->width];
    unsigned int v_u[img->width], v_d[img->width];
    pos = cal;
    for (x = 0; x < img->width; x++) {
        // temp variables
        u_u[x] = img->u[pos][x];
        v_u[x] = img->v[pos][x];
        // write up border
        img->u[0][x] = u_u[x];
        img->v[0][x] = v_u[x];
    }
    for (y = 0; y < img->height - 2; y += 2) {
        pos = pos + 2;
        // prepare variables for next loop
        for (x = 0; x < img->width; x++) {
            u_d[x] = img->u[pos][x];
            v_d[x] = img->v[pos][x];
        }
        for (x = 0; x < img->width; x++) {
            // write un-binned points
            img->u[y + 1][x] = (3 * u_u[x] + 1 * u_d[x]) / 4;
            img->v[y + 1][x] = (3 * v_u[x] + 1 * v_d[x]) / 4;
            img->u[y + 2][x] = (1 * u_u[x] + 3 * u_d[x]) / 4;
            img->v[y + 2][x] = (1 * v_u[x] + 3 * v_d[x]) / 4;
        }
        // prepare variables for next loop
        for (x = 0; x < img->width; x++) {
            u_u[x] = u_d[x];
            v_u[x] = v_d[x];
        }
    }
    // write down border
    for (x = 0; x < img->width; x++) {
        img->u[img->height - 1][x] = u_d[x];
        img->v[img->height - 1][x] = v_d[x];
    }
    img->type &= ~ISS_CSSV;
    img->type &= ~ISS_CALV;
}

Image *img_demosaic_fast(Image *bay) {
    unsigned int x, y, r = 0, g = 0, b = 0;
    Image *img;

    if (!it_type_is_raw(bay->type)) {
        error("input image not in Bayer format");
        return NULL;
    }
    if ((img = new Image()) == NULL)
        return NULL;
    img->width = bay->width;
    img->height = bay->height;
    img->type = bay->type;
    img->depth = bay->depth;

    img->type = (img->type & ~IT_MASK) | IT_RGB;
    if (img_realloc(img))
        return NULL;

    /* main interpolation loop */
    for (y = 0; y < img->height; y++) {
        int line;

        if ((bay->type & IST_ODDV) == 0)
            line = (y & 1) ? B : R;
        else
            line = (y & 1) ? R : B;

        for (x = 0; x < img->width; x++) {
            int b1, b2, b3, b4, b5, b6, b7, b8, b9;
            int b22, b44, b66, b88;

            b1 = bayer_get(bay, x - 1, y - 1);
            b2 = bayer_get(bay, x, y - 1);
            b3 = bayer_get(bay, x + 1, y - 1);
            b4 = bayer_get(bay, x - 1, y);
            b5 = bayer_get(bay, x, y);
            b6 = bayer_get(bay, x + 1, y);
            b7 = bayer_get(bay, x - 1, y + 1);
            b8 = bayer_get(bay, x, y + 1);
            b9 = bayer_get(bay, x + 1, y + 1);

            b22 = bayer_get(bay, x, y - 2);
            b44 = bayer_get(bay, x - 2, y);
            b66 = bayer_get(bay, x + 2, y);
            b88 = bayer_get(bay, x, y + 2);

            switch (color(bay, x, y)) {
                case R:
                    g = (4 * b5 + 2 * (b2 + b4 + b6 + b8) - (b22 + b44 + b66 + b88)) / 8;
                    b = (6 * b5 + 2 * (b1 + b3 + b7 + b9) - 3 * (b22 + b44 + b66 + b88) / 2) / 8;
                    r = b5;
                    break;
                case B:
                    g = (4 * b5 + 2 * (b2 + b4 + b6 + b8) - (b22 + b44 + b66 + b88)) / 8;
                    r = (6 * b5 + 2 * (b1 + b3 + b7 + b9) - 3 * (b22 + b44 + b66 + b88) / 2) / 8;
                    b = b5;
                    break;
                case G:
                    if (line == R) {
                        r = (5 * b5 + 4 * (b4 + b6) - (b1 + b3 + b7 + b9 + b44 + b66)
                                + (b22 + b88) / 2) / 8;
                        b = (5 * b5 + 4 * (b2 + b8) - (b1 + b3 + b7 + b9 + b22 + b88)
                                + (b44 + b66) / 2) / 8;
                    } else {
                        b = (5 * b5 + 4 * (b4 + b6) - (b1 + b3 + b7 + b9 + b44 + b66)
                                + (b22 + b88) / 2) / 8;
                        r = (5 * b5 + 4 * (b2 + b8) - (b1 + b3 + b7 + b9 + b22 + b88)
                                + (b44 + b66) / 2) / 8;
                    }
                    g = b5;
                    break;
                default:
                    assert(0);
                    break;
            }

            img->r[y][x] = clip_nb(img, r);
            img->g[y][x] = clip_nb(img, g);
            img->b[y][x] = clip_nb(img, b);
        }
    }
    return img;
}

Image *img_cef2rgb(Image *img) {
    unsigned int x, y;
    Image *out = new Image(*img);
    if ((img->type & IT_MASK) != IT_CEF) {
        error("cef2rgb: image is not in CEF format");
        return NULL;
    }
    if (out == NULL)
        return NULL;

    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
            int c = img->c[y][x];
            int e = img->e[y][x] - (1 << (img->depth - 1));
            int f = img->f[y][x] - (1 << (img->depth - 1));
            out->r[y][x] = clip_nb(img, c + (3 * e - f) / 2);
            out->g[y][x] = clip_nb(img, c - (e + f) / 2);
            out->b[y][x] = clip_nb(img, c + (3 * f - e) / 2);
        }
    }
    out->type = (out->type & IT_MASK) | IT_RGB;
    return out;
}

Image *img_yuv2rgb(Image *img) {
    unsigned int x, y;
    Image *out = new Image(*img);

    if (img->type != IT_YUV) {
        error("yuv2rgb: image is not in YUV format");
        return NULL;
    }
    if (!(img->y) || !(img->u) || !(img->v)) {
        error("yuv2rgb: image is not in YUV format");
        return NULL;
    }

    if (out == NULL)
        return NULL;

    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
            int u = img->u[y][x] - (1 << (img->depth - 1));
            int v = img->v[y][x] - (1 << (img->depth - 1));
            int l = img->y[y][x];
            out->g[y][x] = clip_nb(img, l - 0.34414 * u - 0.71414 * v);
            out->b[y][x] = clip_nb(img, l + 1.77200 * u);
            out->r[y][x] = clip_nb(img, l + 1.40200 * v);
        }
    }
    out->type = IT_RGB;

    return out;
}

#define min(a, b) a < b ? a : b

int compare(Image* img1, Image* img2) {
    printf("compare...\n");
    int b = img1->width != img2->width;
    b |= img1->height != img2->height;
    b |= img1->depth != img2->depth;
    b |= img1->type != img2->type;
    if (b) {
        img1->info("img1");
        img2->info("img2");
        return -1;
    }
    printf("compare...\n");

    double total = 0;
    int count = 0;
    float maxr = 0;
    float maxg = 0;
    float maxb = 0;
    for (unsigned y = 0; y < img1->height; y++) {
        for (unsigned x = 0; x < img1->width; x++) {
            float dr = abs(img1->r[y][x] - img2->r[y][x]);
            float dg = abs(img1->g[y][x] - img2->g[y][x]);
            float db = abs(img1->b[y][x] - img2->b[y][x]);
            count += (dr != 0);
            count += (dg != 0);
            count += (db != 0);
            float er = dr / (float) min(1, img1->r[y][x]);
            float eg = dg / (float) min(1, img1->g[y][x]);
            float eb = db / (float) min(1, img1->b[y][x]);
            maxr = maxr < er ? er : maxr;
            maxg = maxr < eg ? eg : maxg;
            maxb = maxr < eb ? eb : maxb;
        }
    }
    float size = img1->height * img1->width * 3.0;
    printf("percent of deviated bins: %.1f%%\n", (float) count / size * 100.0);
    printf("avg deviation: %.1f, max deviation: %.1f, %.1f, %.1f\n", total / size, maxr, maxg,
            maxb);
    return count;
}

Image* diff(Image* img1, Image* img2) {
    printf("compare...\n");
    int b = img1->width != img2->width;
    b |= img1->height != img2->height;
    b |= img1->depth != img2->depth;
    b |= img1->type != img2->type;
    if (b) {
        img1->info("img1");
        img2->info("img2");
        return NULL;
    }

    double maxr = 0;
    double maxg = 0;
    double maxb = 0;
    int count = 0;
    for (unsigned y = 0; y < img1->height; y++) {
        for (unsigned x = 0; x < img1->width; x++) {
            double er = abs(img1->r[y][x] - img2->r[y][x]);
            double eg = abs(img1->g[y][x] - img2->g[y][x]);
            double eb = abs(img1->b[y][x] - img2->b[y][x]);
            maxr = maxr < er ? er : maxr;
            maxg = maxg < eg ? eg : maxg;
            maxb = maxb < eb ? eb : maxb;
            count += (er != 0);
            count += (eg != 0);
            count += (eb != 0);
        }
    }

    double max = (1 << img1->depth);
    maxr /= max;
    maxg /= max;
    maxb /= max;

    float size = img1->height * img1->width * 3.0;
    printf("percent of deviated bins: %.1f%%\n", (float) count / size * 100.0);
    printf("max deviation: r:%.1f, g:%.1f, b:%.1f\n", maxr * 100, maxg * 100, maxb * 100);

    if (count == 0) {
        return NULL;
    }

    Image* diff = new Image(*img1);
    for (unsigned y = 0; y < img1->height; y++) {
        for (unsigned x = 0; x < img1->width; x++) {
            double dr = abs(img1->r[y][x] - img2->r[y][x]);
            double dg = abs(img1->g[y][x] - img2->g[y][x]);
            double db = abs(img1->b[y][x] - img2->b[y][x]);
            diff->r[y][x] = dr / maxr;
            diff->g[y][x] = dg / maxg;
            diff->b[y][x] = db / maxb;
        }
    }
    return diff;
}

Image *img_2rgb(Image *img) {
    Image *t = new Image(*img);
    Image *out = NULL;

    if (t->type & IST_MASK) {
        unsigned int x, y;
        int s, n, d;
        if ((t->type & IST_MASK) == IST_LS4_9) {
            s = 4;
            n = 9;
            d = 8;
        } else if ((t->type & IST_MASK) == IST_LS3_9) {
            s = 3;
            n = 9;
            d = 8;
        } else {
            printf("invalid subtype");
            return NULL;
        }
        for (y = 0; y < t->height; y++) {
            for (x = 0; x < t->width; x++) {
                t->b[y][x] = exp_(t->b[y][x], s, n, d);
                if (!it_type_is_raw(t->type)) {
                    t->r[y][x] = exp_(t->r[y][x], s, n, d);
                    t->g[y][x] = exp_(t->g[y][x], s, n, d);
                }
            }
        }
        t->type &= ~IST_MASK;
        t->depth = d;
    }
    if (t->type & ISS_CSSV) {
        if (t->type & ISS_CALV)
            imgyuv_cub_y(t, 1);  // YUV 420
        else
            imgyuv_cus_y(t, 0);  // YUV 422
    }
    if (t->type & ISS_CSSH) {
        if (t->type & ISS_CALV)
            imgyuv_cub_x(t, 1);  // YUV 420
        else
            imgyuv_cus_x(t, 0);  // YUV 422
    }
    switch (t->type & IT_MASK) {
        case IT_RGB:
            out = new Image(*t);
            break;
        case IT_BAY:
            out = img_demosaic_fast(t);
            break;
        case IT_RCCB:
            printf("RCCB demosaic not supported!!");
            return NULL;
            break;
        case IT_YUV:
            out = img_yuv2rgb(t);
            break;
        case IT_RCB:
            printf("RCB to RGB not implemented!!");
            out = new Image(*t);
            break;
        case IT_CEF:
            out = img_cef2rgb(t);
            break;
        case IT_STREAM:
            printf("STREAM decoding not supported!!");
            return NULL;
            break;
        case IT_GRAY:
            printf("GRAY decoding not supported!!");
            return NULL;
            break;
    }

    delete t;
    return out;
}

Image *img_depth(Image *img, unsigned int depth) {
    unsigned int x, y;
    int l, r, round;
    Image *out;

    out = new Image();
    out->width = img->width;
    out->height = img->height;
    out->type = img->type;
    out->depth = depth;
    if (img_realloc(out))
        return NULL;

    assert(img->depth <= MAX_NB /*&& img->depth >= 0*/);
    assert(out->depth <= MAX_NB /*&& out->depth >= 0*/);

    round = l = r = 0;
    if (depth > img->depth)
        l = depth - img->depth;
    else
        r = img->depth - depth;

    if (img->r)
        for (y = 0; y < img->height; y++)
            for (x = 0; x < img->width; x++)
                out->r[y][x] = ((((int) img->r[y][x]) << l) + round) >> r;
    if (img->g)
        for (y = 0; y < img->height; y++)
            for (x = 0; x < img->width; x++)
                out->g[y][x] = ((((int) img->g[y][x]) << l) + round) >> r;
    if (img->b)
        for (y = 0; y < img->height; y++)
            for (x = 0; x < img->width; x++)
                out->b[y][x] = ((((int) img->b[y][x]) << l) + round) >> r;

    return out;
}

int Image::write(char *fn) {
    char *ext = strrchr(fn, '.');
    int r = 1;

    if (ext == NULL) {
        if (strcmp(fn, "-") == 0) { //write to stdout
            fn = NULL;
        } else {
            error("Cannot determine file extension");
            return 1;
        }
    } else {
        if (fn[0] == '-' && fn[1] == '.') { //write to stdout
            fn = NULL;
        }
    }

    Image *t = img_2rgb(this);
    if (!t)
        return 1;

    t = img_depth(t, 8);
    r = write_bmp(t, fn);
    delete t;

    return r;
}

const char *img_type_names[] = { "RGB    ", "bayer  ", "RCCB   ", "YUV    ", "RCB    ", "CEF    ",
        "stream ", "gray   " };

void Image::info(const char *text) {
    printf("%-20s: size %dx%d (bits %d) %s", text, width, height, depth,
            img_type_names[type & IT_MASK]);

    if ((type & IST_MASK) == IST_LS4_9)
        printf(" log s4.9 space");
    if ((type & IST_MASK) == IST_LS3_9)
        printf(" log s3.9 space");
    if (type & IST_ODDH)
        printf(" horizontal flip");
    if (type & IST_ODDV)
        printf(" verical flip");

    if (type & ISS_CSSH)
        printf(" chroma subsampling in x dimension");
    if (type & ISS_CSSV)
        printf(" chroma subsampling in y dimension");
    printf("\n");
}

Image::Image(const unsigned int width, const unsigned int height, const unsigned char depth,
        const unsigned int type, const short *rgb) :
        width(width), height(height), depth(depth), type(type), r(NULL), g(NULL), b(NULL) {

    img_realloc(this);

    int mask = (1 << depth) - 1;
    if (depth == 8) {
        printf("8 bit srgb\n");
        unsigned char *p = (unsigned char *)rgb;
        for (unsigned y = 0; y < height; ++y) {
            unsigned short *_r = r[y];
            unsigned short *_g = g[y];
            unsigned short *_b = b[y];
            for (unsigned x = 0; x < width; x++) {
                _r[x] = mask & (unsigned short)(*p++);
                _g[x] = mask & (unsigned short)(*p++);
                _b[x] = mask & (unsigned short)(*p++);
                p++;
            }
        }
    } else {
        printf("--  %d bit srgb\n", depth);
        if (rgb != NULL && type == IT_RGB) {
            for (unsigned y = 0; y < height; ++y) {
                unsigned short *_r = r[y];
                unsigned short *_g = g[y];
                unsigned short *_b = b[y];
                for (unsigned x = 0; x < width; x++) {
                    _r[x] = mask & (unsigned short)(*rgb++);
                    _g[x] = mask & (unsigned short)(*rgb++);
                    _b[x] = mask & (unsigned short)(*rgb++);
                }
            }
        }
    }
}

Image::Image(const Image& ref) {
    width = ref.width;
    height = ref.height;
    type = ref.type;
    depth = ref.depth;
    r = g = b = NULL;

    img_realloc(this);

    int size = height * width * sizeof(short);
    if (ref.r)
        memcpy(r[0], ref.r[0], size);
    if (ref.g)
        memcpy(g[0], ref.g[0], size);
    if (ref.b)
        memcpy(b[0], ref.b[0], size);
}

Image::~Image() {
    free2D(r);
    free2D(g);
    free2D(b);
}

/********************************************************************
 * Image processing algos
 ********************************************************************/
Image *img_demosaic_bilinear(Image *bay) {
    unsigned int x, y, r, g, b;
    Image *img;

    if (!it_type_is_raw(bay->type)) {
        error("input image not in Bayer format");
        return NULL;
    }
    img = new Image(bay->width, bay->height, bay->depth, IT_RGB);
    if (img == NULL)
        return NULL;

    /* main interpolation loop */
    for (y = 0; y < img->height; y++) {
        int line;

        if ((bay->type & IST_ODDV) == 0)
            line = (y & 1) ? B : R;
        else
            line = (y & 1) ? R : B;

        for (x = 0; x < img->width; x++) {
            int b1, b2, b3, b4, b5, b6, b7, b8, b9;

            b1 = bayer_get(bay, x - 1, y - 1);
            b2 = bayer_get(bay, x, y - 1);
            b3 = bayer_get(bay, x + 1, y - 1);
            b4 = bayer_get(bay, x - 1, y);
            b5 = bayer_get(bay, x, y);
            b6 = bayer_get(bay, x + 1, y);
            b7 = bayer_get(bay, x - 1, y + 1);
            b8 = bayer_get(bay, x, y + 1);
            b9 = bayer_get(bay, x + 1, y + 1);

            if (color(bay, x, y) != G) {
                int a5;
                /* interpolate green */
                g = (b2 + b4 + b6 + b8 + 2) / 4;

                /* interpolate chroma on Bayer opposite chroma pixels*/
                a5 = (b1 + b3 + b7 + b9 + 2) / 4;

                r = (line == R) ? b5 : a5;
                b = (line == B) ? b5 : a5;

            } else { /* interpolate chroma on Bayer green pixels */
                int a5a, a5b;
                a5a = (b4 + b6 + 1) / 2;
                a5b = (b2 + b8 + 1) / 2;
                r = (line == R) ? a5a : a5b;
                g = b5;
                b = (line == B) ? a5a : a5b;
            }
            img->r[y][x] = r;
            img->g[y][x] = g;
            img->b[y][x] = b;
        }
    }
    return img;
}

Image* x_img_demosaic_bilinear(Image *bay) {
    unsigned int r, g, b;
    Image *img;

    if (!it_type_is_raw(bay->type)) {
        error("input image not in Bayer format");
        return NULL;
    }

    img = new Image(bay->width, bay->height, bay->depth, IT_RGB);
    if (img == NULL)
        return NULL;

    // main interpolation loop
    bool br = (bay->type & IST_ODDV) == 0;
    printf("br = %d", br);
    for (unsigned y = 0; y < img->height; y++) {
        int line = br ? ((y & 1) ? B : R) : ((y & 1) ? R : B);
        for (unsigned x = 0; x < img->width; x++) {
            int b1, b2, b3, b4, b5, b6, b7, b8, b9;

            b1 = bayer_get(bay, x - 1, y - 1);
            b2 = bayer_get(bay, x, y - 1);
            b3 = bayer_get(bay, x + 1, y - 1);
            b4 = bayer_get(bay, x - 1, y);
            b5 = bayer_get(bay, x, y);
            b6 = bayer_get(bay, x + 1, y);
            b7 = bayer_get(bay, x - 1, y + 1);
            b8 = bayer_get(bay, x, y + 1);
            b9 = bayer_get(bay, x + 1, y + 1);

            if (color(bay, x, y) != G) {
                int a5;
                /* interpolate green */
                g = (b2 + b4 + b6 + b8 + 2) / 4;

                /* interpolate chroma on Bayer opposite chroma pixels*/
                a5 = (b1 + b3 + b7 + b9 + 2) / 4;

                r = (line == R) ? b5 : a5;
                b = (line == B) ? b5 : a5;

            } else { /* interpolate chroma on Bayer green pixels */
                int a5a, a5b;
                a5a = (b4 + b6 + 1) / 2;
                a5b = (b2 + b8 + 1) / 2;
                r = (line == R) ? a5a : a5b;
                g = b5;
                b = (line == B) ? a5a : a5b;
            }
            img->r[y][x] = r;
            img->g[y][x] = g;
            img->b[y][x] = b;

            if (y == 10 && x < 32 && x > 2) {
                printf("b:%d, g: %d, r:%d; %d,%d,%d,%d,%d,%d,%d,%d,%d\n", b, g, r, b1,b2,b3,b4,b5,b6,b7,b8,b9);
            }
        }
    }

    return img;
}

int median5x5(int p[25]) {
    int x, y, i = 0;
    int t[25];
    int c = 25;
    for (x = 0; x < 25; x++) {
        if (p[x] < 0)
            c--;
        else {
            t[i] = p[x];
            i++;
        }
    }
    for (x = 0; x < c - 1; x++) {
        int max = t[x], max_inx = x;
        for (y = x + 1; y < c; y++) {
            if (t[y] > max) {
                max = t[y];
                max_inx = y;
            }
        }
        t[max_inx] = t[x];
        t[x] = max;
    };
    return t[c / 2];
}

Image *img_median5(Image *img) {
    int x, y;
    Image *out = new Image(*img);

    if (out == NULL)
        return NULL;
    for (y = 0; y < (int) img->height; y++) {
        for (x = 0; x < (int) img->width; x++) {
            int p[25];
            int x0, y0;
            for (y0 = -2; y0 < 3; y0++) {
                for (x0 = -2; x0 < 3; x0++) {
                    if ((y + y0 >= 0) && (y + y0 < (int) img->height) && (x + x0 >= 0)
                            && (x + x0 < (int) img->width)) {
                        p[((y0 + 2) * 5) + (x0 + 2)] = img->b[y + y0][x + x0];
                    } else {
                        p[((y0 + 2) * 5) + (x0 + 2)] = -1;
                    }
                }
            }

            if (it_type_is_raw(img->type)) {
                for (x0 = 1; x0 < 25; x0 += 2)
                    p[x0] = -1;
                p[6] = -1;
                p[8] = -1;
                p[16] = -1;
                p[18] = -1;
                out->b[y][x] = median5x5(p);
            } else {
                out->b[y][x] = median5x5(p);
                for (y0 = -2; y0 < 3; y0++) {
                    for (x0 = -2; x0 < 3; x0++) {
                        if ((y + y0 >= 0) && (y + y0 < (int) img->height) && (x + x0 >= 0)
                                && (x + x0 < (int) img->width)) {
                            p[((y0 + 2) * 5) + (x0 + 2)] = img->g[y + y0][x + x0];
                        } else {
                            p[((y0 + 2) * 5) + (x0 + 2)] = -1;
                        }
                    }
                }
                out->g[y][x] = median5x5(p);
                for (y0 = -2; y0 < 3; y0++) {
                    for (x0 = -2; x0 < 3; x0++) {
                        if ((y + y0 >= 0) && (y + y0 < (int) img->height) && (x + x0 >= 0)
                                && (x + x0 < (int) img->width)) {
                            p[((y0 + 2) * 5) + (x0 + 2)] = img->r[y + y0][x + x0];
                        } else {
                            p[((y0 + 2) * 5) + (x0 + 2)] = -1;
                        }
                    }
                }
                out->r[y][x] = median5x5(p);
            }
        }
    }

    return out;
}

/* convert RGB image to Bayer */
Image *img_tobayer(Image *img) {
    Image *out = new Image();
    unsigned int x, y;
    if (it_type_is_raw(img->type)) {
        error("Image is already a Bayer image!");
        return NULL;
    }

    if (!out)
        return NULL;
    out->width = img->width;
    out->height = img->height;
    out->type = img->type;
    out->depth = img->depth;

    out->type = (out->type & ~IT_MASK) | IT_BAY;
    if (img_realloc(out))
        return NULL;

    /* main interpolation loop */
    for (y = 0; y < img->height; y++)
        for (x = 0; x < img->width; x++)
            switch (color(out, x, y)) {
                case R:
                    out->b[y][x] = img->r[y][x];
                    break;
                case G:
                    out->b[y][x] = img->g[y][x];
                    break;
                case B:
                    out->b[y][x] = img->b[y][x];
                    break;
            }
    out->r = out->g = NULL;
    return out;
}

/* image conversion */
Image *img_gamma(Image *img, double from, double to) {
    unsigned int x, y;
    short table[1 << MAX_NB];
    Image *out;

    out = new Image();
    out->width = img->width;
    out->height = img->height;
    out->type = img->type;
    out->depth = img->depth;
    if (img_realloc(out))
        return NULL;

    assert(img->depth <= MAX_NB);
    const double m = (1 << img->depth);
    int i;

    for (i = 0; i < (1 << img->depth); i++) {
        double v = i / m;
        if (from == 0) {
            if (v > 0.04045)
                v = pow((v + 0.055) / 1.055, 2.4);
            else
                v = v / 12.92;
        } else
            v = pow(v, from);

        if (to == 0) {
            if (v <= 0.0031308)
                v = 12.92 * v;
            else
                v = 1.055 * pow(v, 1. / 2.4) - 0.055;
        } else
            v = pow(v, 1. / to);
        table[i] = clip_nb(img, (int) (m * v));
    }

    if (img->r)
        for (y = 0; y < img->height; y++)
            for (x = 0; x < img->width; x++)
                out->r[y][x] = table[clip_nb(img, img->r[y][x])];
    if (img->g)
        for (y = 0; y < img->height; y++)
            for (x = 0; x < img->width; x++)
                out->g[y][x] = table[clip_nb(img, img->g[y][x])];
    if (img->b)
        for (y = 0; y < img->height; y++)
            for (x = 0; x < img->width; x++)
                out->b[y][x] = table[clip_nb(img, img->b[y][x])];

    return out;
}

