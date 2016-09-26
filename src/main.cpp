/*
 * main.cpp
 *
 *  Created on: Mar 25, 2013
 *      Author: ab
 */
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>

#include "image.h"
#include "debug.h"
#include "Params.h"
#include "Pipeline.h"
#include <unistd.h>

#define KERNELS_PATH "kernels"

static bool DEBUG = false;

using namespace std;

void unpack(int nWidth, int nHeight, uint8_t *pOut, uint8_t *pIn) {
    int instride = (nWidth * 10 + 7) / 8;
    int outstride = nWidth * 2;

    if (!pOut || !pIn)
        return;

    for (int i = 0; i < nHeight; ++i) {
        uint16_t *pout = (uint16_t *) (pOut + outstride * i);
        uint8_t *pin = pIn + instride * i;
        uint32_t dw;
        int j;

        //  4 pixels in 5 bytes
        for (j = 0; j < ((nWidth - 4) & ~3); j += 4) {
            dw = *((uint32_t*) pin);
            pout[0] = (uint16_t) dw & 0x3FF;
            pout[1] = (uint16_t)(dw >> 10) & 0x3FF;
            pout[2] = (uint16_t)(dw >> 20) & 0x3FF;
            pout[3] = (uint16_t)((pin[4] << 2) | (dw >> 30)) & 0x3FF;
            pin += 5;
            pout += 4;
        }
        //  Leftovers
        switch (nWidth - j) {
            case 1:
                dw = *((uint32_t*) pin);
                pout[0] = (uint16_t) dw & 0x3FF;
                break;
            case 2:
                dw = *((uint32_t*) pin);
                pout[0] = (uint16_t) dw & 0x3FF;
                pout[1] = (uint16_t)(dw >> 10) & 0x3FF;
                break;
            case 3:
                dw = *((uint32_t*) pin);
                pout[0] = (uint16_t) dw & 0x3FF;
                pout[1] = (uint16_t)(dw >> 10) & 0x3FF;
                pout[2] = (uint16_t)(dw >> 20) & 0x3FF;
                break;
        }
    }
}

void unpackCrbc(int nWidth, int nHeight, uint8_t *pOut, uint8_t *pIn) {
    int instride = (nWidth * 10 + 7) / 8;
    int outstride = nWidth * 2;

    pOut -= 2;
    for (int i = 0; i < nHeight; ++i) {
        //  4 pixels in 5 bytes
        uint16_t *pout = (uint16_t *) (pOut + outstride * i);
        uint8_t *pin = pIn + instride * i;
        for (int j = 0; j <= ((nWidth - 4) & ~3); j += 4) {
            uint32_t dw = *((uint32_t*) pin);
            if (i + j) {
                pout[0] = (uint16_t) dw & 0x3FF;
            }
            pout[1] = (uint16_t)(dw >> 10) & 0x3FF;
            pout[2] = (uint16_t)(dw >> 20) & 0x3FF;
            if (nWidth - j > 4) {
                pout[2] = (uint16_t)(dw >> 20) & 0x3FF;
                pout[3] = (uint16_t)((pin[4] << 2) | (dw >> 30)) & 0x3FF;
            } else {
                pout[3] = pout[2] = 0;
            }
            pin += 5;
            pout += 4;
            //  Leftovers
            if (nWidth - j < 4)
                trace("%d", nWidth - j);
            switch (nWidth - j) {
                case 1:
                    dw = *((uint32_t*) pin);
                    pout[0] = (uint16_t) dw & 0x3FF;
                    break;
                case 2:
                    dw = *((uint32_t*) pin);
                    pout[0] = (uint16_t) dw & 0x3FF;
                    pout[1] = (uint16_t)(dw >> 10) & 0x3FF;
                    break;
                case 3:
                    dw = *((uint32_t*) pin);
                    pout[0] = (uint16_t) dw & 0x3FF;
                    pout[1] = (uint16_t)(dw >> 10) & 0x3FF;
                    pout[2] = (uint16_t)(dw >> 20) & 0x3FF;
                    break;
            }
        }
    }
}

bool read(void *buf, void *appData) {
    if (DEBUG) trace("read");
    static int COUNT = 0;
    int count = COUNT++;
    RCCB *rccb = (RCCB *)appData;
    int frameIndex = count % rccb->nFrames;
    int frameSize = (rccb->width * rccb->height * rccb->depth) / 8;
    int off = frameIndex * frameSize;
    if (DEBUG) trace("== read called == : count = %d, index = %d, off = %d, buf = %p", count, frameIndex, off, buf);
    count %= rccb->nFrames;
    if (count >= rccb->nFrames) {
        trace("== read done (no more frames)== : count = %d, index = %d, off = %d, buf = %p", count, frameIndex, off, buf);
        return false;
    }

    unpackCrbc(rccb->width, rccb->height, (uint8_t*)buf, (uint8_t*)(rccb->pixels) + off);
    if (DEBUG) trace("== read done == : count = %d, buf = %p", count, buf);
    return true;
}

int updateTextureBuffer(void *pvData, int width, int height);

bool write(void *buf, void *appData) {
    static int COUNT = 0;
    int count = COUNT++;
    RCCB *rccb = (RCCB *)appData;

    if (DEBUG) trace("== write called == : count = %d, buf = %p", count, buf);
    if (!rccb->fn_out_srgb) return false;

    rccb->nAggregate = updateTextureBuffer(buf, rccb->width, rccb->height);
    return true;

    char fname[256];
    sprintf(fname, "video/%s_%d.bmp", rccb->fn_out_srgb, count);
    // Create and write the output image
    Image *outImg = new Image(rccb->width, rccb->height, 8, IT_RGB, (const short *)buf);
    if (DEBUG) trace("Writing sRGB image %s", fname);
    if (outImg->depth > 8)
        outImg->write(fname);   // sRGB
    else if (write_bmp(outImg, fname))
        trace("Error writing output file");

    delete outImg;
    if (DEBUG) trace("== write done == : count = %d, buf = %p", count, buf);
    return true;
}

bool handler(void *appData) {
    if (DEBUG) trace("main: handler");
    // TODO: balance here?
    return true;
}

int glut(int argc, char *argv[]);

void* glutStart(void *data) {
    trace("==============> starting glut ...");
    glut(0, 0);
}

void* pipeStart(void *data) {
    trace("==============> starting pipe ...");
    Pipeline* pipeline = (Pipeline *)data;
    pipeline->start(&read, &write, &handler);

}

int example(int argc, char *argv[]);
int main(int argc, char *argv[]) {
    //example(argc, argv);
    const int N = 10;

    RCCB rccb;

    // 1. Read command line arguments
    rccb.parse_args(argc, argv);
    if (!rccb.fn_in)
        exit(-1);

    // 2. Read input image
    if (rccb.width == 0 || rccb.height == 0) {
        trace("reading single image");
        Image* inImg = Image::read(rccb.fn_in);
        trace("Reading '%s'\n", rccb.fn_in);
        if (!inImg) {
            trace("Cannot read file '%s'\n", rccb.fn_in);
            exit(1);
        }
        rccb.width = inImg->width;
        rccb.height = inImg->height;
        rccb.depth = rccb.fd;
        rccb.nFrames = 1;
        rccb.pixels = new short[inImg->width * inImg->height];
        for (unsigned y = 0; y < inImg->height; ++y) {
            memcpy(&rccb.pixels[y * inImg->width], inImg->b[y], inImg->width * sizeof(short));
        }
    } else {
        trace("reading video");
        int size = (rccb.width * rccb.height * rccb.depth) / 8;
        // read file
        ifstream file(rccb.fn_in, ios::in | ios::binary | ios::ate);
        if (file.is_open()) {
            ifstream::pos_type fsize = file.tellg();
            if (fsize % size != 0) {
                // Error
                trace("Wrong file size!");
            }
            rccb.nFrames = fsize / size;
            trace("file size = %d, frame size = %d, frames = %d", (int )fsize, size, rccb.nFrames);
            //rccb.nFrames = N + 1;
            rccb.pixels = new short[fsize / sizeof(short)];
            file.seekg(0, ios::beg);
            file.read((char*) rccb.pixels, fsize);
            file.close();
        } else {
            trace("Failed to open file");
        }
        rccb.type = IT_CRBC;
    }

    // 3. Create the pipeline
    Pipeline* pipeline = new Pipeline;
    bool end = !pipeline->init(rccb.width, rccb.height, rccb.type, rccb.depth, KERNELS_PATH, &rccb, &rccb   );
    if (end) {
        trace("Failed to initilize pipeline");
        return EXIT_FAILURE;
    }

    short *in = new short[rccb.width * rccb.height * rccb.nFrames + 1];

    if (rccb.nFrames == 1) {
        memcpy(in, rccb.pixels, rccb.width * rccb.height * sizeof(short));
    } else {
        unpack(rccb.width, rccb.height * rccb.nFrames, (uint8_t*) in, (uint8_t*) rccb.pixels);
    }

//    int n = rccb.width * rccb.height;
//    for (int j = 0; j < rccb.nFrames - N; j++) {
//        int off = j * n;
//        for (int y = 0; y < rccb.height; y++) {
//            for (int x = 0; x < rccb.width; x++) {
//                int xy = off + y * rccb.width + x;
//                int s[100]= {0};
//                for (int k = 0; k < N; k++) {
//                    s[k] = in[xy + k * n];
//                }
//                in[xy] = median(s, N);
//            }
//        }
//    }

    // 4. Process image
    timestamp("total time");
    pipeline->blackBalance(in);
    pipeline->whiteBalance(in);

    pthread_t pipelineThread;
    pthread_create( &pipelineThread, NULL, &pipeStart, (void*)pipeline);

    glut(argc, argv);

    char *out = NULL;
    out = (char *)pipeline->processFrame(in, out);
    timestamp("total time");

    // 5. Create and write the output image
    Image *outImg = new Image(rccb.width, rccb.height, 8, IT_RGB, (const short *)out);
    if (rccb.fn_out_srgb) {
        trace("Writing sRGB image %s", rccb.fn_out_srgb);
        if (outImg->depth > 8)
            outImg->write(rccb.fn_out_srgb);   // sRGB
        else if (write_bmp(outImg, rccb.fn_out_srgb))
            trace("Error writing output file");

    }

    pipeline->processFrame(NULL, out);

    delete rccb.pixels;
    delete in;
    delete outImg;

    trace("Exit OK");
    return 0;
}

