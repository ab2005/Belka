#include <errno.h>
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <thread>
#include <chrono>

#include <android_native_app_glue.h>
#include <android/window.h>

#include "debug.h"
#include "Activity.h"
#include "Pipeline.h"

#define KERNELS_PATH "/sdcard/rccb_video"
#define VIDEO_RAW_FILE "/sdcard/rccb_video/video.raw"

using namespace std;

static bool DEBUG = false;
extern int n_frames;

typedef struct {
    struct android_app* app;
    bool animating;
    short *inImg;
    char *outImg;
    Pipeline *pipeline;
    RCCB *rccbParams;
    int frameIndex;
} Engine;

static void engine_term_display(Engine* engine) {
  //power_measure_stop_tag_cardinality("Clarity+", n_frames);
    trace("engine_term_display");
    engine->animating = 0;
//    engine->outImg = NULL;
}

// Process all pending main thread events.
bool handleMainThreadEvents(void *appData) {
    struct android_app* app = (struct android_app *)appData;
    int ident;
    int events;
    struct android_poll_source* source;
    Engine* engine = (Engine*)app->userData;

    // If not animating, we will block forever waiting for events.
    // If animating, we loop until all events are read
    while ((ident = ALooper_pollAll(engine->animating ? 0 : -1, NULL, &events, (void**) &source)) >= 0) {
        if (source != NULL) {
            source->process(app, source);
        }
        // Check if we are exiting.
        if (app->destroyRequested != 0) {
            trace("Engine thread destroy requested!");
            engine_term_display (engine);
            // TODO: cleanup
            return false;
        }
    }
    return true;
}

static void unpackCrbc(int nWidth, int nHeight, uint8_t *pOut, uint8_t *pIn) {
    int instride = (nWidth * 10 + 7) / 8;
    int outstride = nWidth * 2;

    pOut -= 2;

    for (int i = 0; i < nHeight; ++i) {//, pOut += outstride, pin += instride) {
        //  4 pixels in 5 bytes
        uint16_t *pout = (uint16_t *) (pOut + outstride * i);
        uint8_t *pin = pIn + instride * i;
        for (int j = 0; j < ((nWidth - 4) & ~3); j += 4) {
            uint32_t dw = *((uint32_t*) pin);
            if (i > 0 && j > 0) {
                pout[0] = (uint16_t) dw & 0x3FF;
                pout[1] = (uint16_t)(dw >> 10) & 0x3FF;
            }
            pout[2] = (uint16_t)(dw >> 20) & 0x3FF;
            pout[3] = (uint16_t)((pin[4] << 2) | (dw >> 30)) & 0x3FF;
            pin += 5;
            pout += 4;
        }
    }
}

bool read(void *buf, void* userData) {
    struct android_app* app = (struct android_app*)userData;
    Engine* engine = (Engine*)app->userData;
    RCCB *rccb = engine->rccbParams;
    // read rccb from disk
    rccb->readParams();

    static int COUNT = 0;
    int count = COUNT++;

    int frameIndex = count % rccb->nFrames;
    int frameSize = (rccb->width * rccb->height * rccb->depth) / 8;
    int off = frameIndex * frameSize;
    if (DEBUG) trace("== read called == : count = %d, index = %d, off = %d, buf = %p", count, frameIndex, off, buf);
    unpackCrbc(rccb->width, rccb->height, (uint8_t*)buf, (uint8_t*)rccb->pixels + off);

    Pipeline *pipe = engine->pipeline;
    pipe->blackBalance((short *)buf);
    pipe->whiteBalance((short *)buf);
    if (DEBUG) trace("== read done == : count = %d, buf = %p", count, buf);
    return true;
}

static void fill_buffer(ANativeWindow_Buffer* buffer, char *pixels, int width, int height, int x, int y) {
    if (pixels == NULL) {
        trace("pixels == NULL");
        return;
    }

    //timestamp("\t\tblit");
    int h = buffer->height;
    if (h > height) h = height;

    int w = buffer->width;
    if (w > width) w = width;

    char *in = pixels + (y * width + x) * 4;
    char *out = (char *)buffer->bits;
    for (int i = 0; i < h; i++, in += width * 4, out += buffer->stride * 4) {
        memcpy(out, in, w * 4);
    }
    //timestamp("\t\tblit");
}

static void engine_update_window(Engine *engine, int dx, int dy, bool fillBuffer = false) {
    if (DEBUG) timestamp("\tengine_update_window");
    static int x = 0, y = 0;
    ANativeWindow_Buffer buffer;
    RCCB *rccb = engine->rccbParams;

    if (ANativeWindow_lock(engine->app->window, &buffer, NULL) < 0) {
        trace("Unable to lock window buffer");
        timestamp("\tengine_update_window");
        return;
    }

    x += dx;
    if (x >= (rccb->width - buffer.width) ) {
        x = rccb->width - buffer.width;
    }

    if (x < 0)  {
        x = 0;
    }
    y += dy;

    if (y >= (rccb->height - buffer.height)) {
        y = rccb->height - buffer.height;
    }

    if (y < 0)  {
        y = 0;
    }

    if (fillBuffer) {
        fill_buffer(&buffer, (char*) engine->outImg, rccb->width, rccb->height, x, y);
    }

    ANativeWindow_unlockAndPost(engine->app->window);
    if (DEBUG) timestamp("\tengine_update_window");
}

bool write(void *buf, void* userData) {
    static int off = 0;
    struct android_app* app = (struct android_app*)userData;
    Engine* engine = (Engine*)app->userData;

    static int COUNT = 0;
    int count = COUNT++;

//    if (DEBUG) timestamp("== write");
//    int siz = engine->rccbParams->height * engine->rccbParams->width * 2 * 2;
//    char *p = (char *)buf;
//    memcpy(engine->outImg + siz * off, p + siz * off, siz);
//    memcpy(engine->outImg, buf, siz);
    engine->outImg = (char *)buf;
    engine_update_window(engine, 0, 0, true);
//    if (DEBUG) timestamp("== write");
    //off = (off + 1) % 2;
    return true;
}

//static void* updateDisplay(void *data) {
//    //if (DEBUG)
//        trace("start update display thread");
//    Engine* engine = (Engine*)data;
//    while(engine->animating) {
//        if (DEBUG)
//            timestamp("update display");
//        engine_update_window(engine, 0, 0, true);
//        if (DEBUG)
//            timestamp("update display");
//    }
//    return 0;
//}

static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    Engine* engine = (Engine*) app->userData;

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        static int x0 = 0, y0 = 0;
        static long t = nanotime();
        long long t1 = nanotime() - t;
        size_t pointerCount = AMotionEvent_getPointerCount(event);
        if (pointerCount > 1) {
            if (t1 > 1000*1000*50) {
                t = nanotime();
                engine->animating = !engine->animating;
            }
            return 1;
        }

        for (size_t i = 0; i < pointerCount; i++) {
            size_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            size_t pointerIndex = i;
            float x = AMotionEvent_getX(event, pointerIndex);
            float y = AMotionEvent_getY(event, pointerIndex);
            if (action == AMOTION_EVENT_ACTION_DOWN) {
                x0 = x;
                y0 = y;
            } else if (action == AMOTION_EVENT_ACTION_UP) {
            } else if (action == AMOTION_EVENT_ACTION_MOVE) {
                //engine->animating = false;
                if (t1 > 1000*1000*50) {
                    if (DEBUG) trace("t1=%d (ms)", t1/1000000);
                    engine_update_window(engine, x0 - x, y0 - y);
                    t = nanotime();
                }
                x0 = x;
                y0 = y;
            }
        }
        return 1;
    }
    return 0;
}

static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = (Engine*) app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            trace("APP_CMD_INIT_WINDOW");
            if (engine->app->window != NULL) {
                int32_t width = ANativeWindow_getWidth(app->window);
                int32_t height = ANativeWindow_getHeight(app->window);
                trace("w:%d, h:%d", width, height);
                ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_FULLSCREEN,
                        AWINDOW_FLAG_FORCE_NOT_FULLSCREEN);
                ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
                if (ANativeWindow_setBuffersGeometry(app->window, width, height,
                        WINDOW_FORMAT_RGBX_8888) < 0) {
                    trace("Failed to setBufferGeometry");
                }
                engine->animating = true;
                // TODO: show splash screen
                n_frames = 0;
               // power_measure_start();
           }
            break;
        case APP_CMD_TERM_WINDOW:
            engine_term_display(engine);
            break;
        case APP_CMD_LOST_FOCUS:
            engine->animating = false;
            engine->pipeline->stop();
            break;
    }
}

void android_main(struct android_app* state) {
    Engine engine;
    engine.animating = false;

    Pipeline *pipeline = new Pipeline();
    RCCB rccb;
    engine.rccbParams = &rccb;
    engine.pipeline = pipeline;

    rccb.width = 4016;
    rccb.height = 3016;
    char fname[] = VIDEO_RAW_FILE;
    rccb.fn_in = fname;
    rccb.depth = 10;

    // Make sure glue isn't stripped.
    app_dummy();

    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // 2. Read input image
//    if (rccb.width == 0 || rccb.height == 0) {
//        trace("reading single image");
//        Image* inImg = Image::read(rccb.fn_in);
//        trace("Reading '%s'\n", rccb.fn_in);
//        if (!inImg) {
//            trace("Cannot read file '%s'\n", rccb.fn_in);
//            exit(1);
//        }
//        rccb.width = inImg->width;
//        rccb.height = inImg->height;
//        rccb.depth = inImg->depth;
//        rccb.nFrames = 1;
//        rccb.pixels = new short[inImg->width * inImg->height];
//        for (unsigned y = 0; y < inImg->height; ++y) {
//            memcpy(&rccb.pixels[y * inImg->width], inImg->b[y], inImg->width * sizeof(short));
//        }
//        delete inImg;
//    } else {
//        trace("reading video");
//        rccb.readParams();
//        trace("width = %d, height = %d, depth = %d", rccb.width, rccb.height, rccb.depth);
//        int size = (rccb.width * rccb.height * rccb.depth) / 8;
//        // read file
//        FILE *file = fopen(rccb.fn_in, "r+b");
//        if (file == NULL) {
//            trace("Can not read file %s", rccb.fn_in);
//            return;
//        }
//
//        fseek(file, 0, SEEK_END);
//        long fsize = ftell(file);
//        rewind(file);
//        rccb.nFrames = fsize / size;
//        trace("file size = %d, frame size = %d, frames = %d", (int )fsize, size, rccb.nFrames);
//
//        // limited to avoid low memory issue
//            if (rccb.nFrames > 10) {
//                rccb.nFrames = 10;
//            }
//        size *= rccb.nFrames;
//        rccb.pixels = new short[size / sizeof(short)];
//        long res = fread(rccb.pixels, 1, size, file);
//        fclose(file);
//        if (res != size) {
//            trace("Error reading file res = %ul", rccb.fn_in);
//            return;
//        }
//        rccb.type = IT_CRBC;
//    }

    engine.outImg = new char[rccb.width * rccb.height * 4];

    trace("init pipeline...");
    timestamp("init pipeline");
//    bool end = !pipeline->init(rccb.width, rccb.height, rccb.type, rccb.depth, KERNELS_PATH, &rccb, state);
    bool end = !pipeline->init(1024*3, 1024*2, rccb.type, rccb.depth, KERNELS_PATH, &rccb, state);
    if (end) {
        trace("Failed to initilize pipeline");
        return;
    }

    timestamp("init pipeline");

    while (1) {
         int ident;
         int events;
         struct android_poll_source* source;
         while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events, (void**) &source)) >= 0) {
             if (source != NULL) {
                 source->process(state, source);
             }
             if (state->destroyRequested != 0) {
                 trace("Engine thread destroy requested!");
                 engine_term_display(&engine);
                 delete pipeline;
                 delete rccb.pixels;
                 delete engine.outImg;
                 return;
             }
         }
         if (engine.animating) {
             trace("start pipeline...");
             timestamp("start pipeline");

             // TODO: start display renderer thread
//             pthread_t displayRenderer;
//             pthread_create( &displayRenderer, NULL, &updateDisplay, &engine);
             engine.pipeline->start(&read, &write, &handleMainThreadEvents);

             engine.animating = false;
//             pthread_join(displayRenderer, 0);
             trace("stopped pipeline");
             timestamp("start pipeline");
         }
    }
}
