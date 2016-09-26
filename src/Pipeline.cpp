/*
 * Pipeline.cpp
 *
 *  Created on: Mar 25, 2013
 *      Author: ab
 */
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <math.h>
#include <algorithm>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "Pipeline.h"
#include "debug.h"
#include "image.h"

#include "min_max_abs_sqr.h"

#ifdef __MACH__
#   include <mach/clock.h>
#   include <mach/mach.h>
#endif

#ifndef NOANDROID
#   include <android/log.h>
#endif

using namespace std;

static bool DEBUG = false;

static bool TRACE_TIME = true;
static unsigned long WAIT_TICK = 1000 * 1; // 100ms
int n_frames;

void Pipeline::floats(int width, int height, bool (*looper)(void *)) {
    int imgWidth = width / 4;
    int imgHeight = height * 3 / 2;
    cl::Buffer in1 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 4 * imgWidth * imgHeight);
    cl::Buffer in2 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 4 * imgWidth * imgHeight);
    cl::Buffer out = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 4 * imgWidth * imgHeight);
    cl::ImageFormat rgba8 = cl::ImageFormat(CL_RGBA, CL_UNORM_INT8);
    cl::Image2D yuv = cl::Image2D(*m_context, CL_MEM_READ_WRITE, rgba8, imgWidth, imgHeight);
    cl::Image2D yuv_out = cl::Image2D(*m_context, CL_MEM_READ_WRITE, rgba8, imgWidth, imgHeight);
    cl::Image2D yuvd = cl::Image2D(*m_context, CL_MEM_READ_WRITE, rgba8, imgWidth, imgHeight);
//    imgWidth = 1024 * 4; imgHeight = 1024 * 4;
//    trace("create image: %d x %d", imgWidth, imgHeight);
//    cl::Image2D img = cl::Image2D(*m_context, CL_MEM_READ_WRITE, rgba8, imgWidth, imgHeight);

    // profile
    trace("initializing yuv image...");
    int input = 250;
    int inputUV = 128;
    trace("Y = %d, UV = %d", input, inputUV);

    cl::size_t<3> origin, region;
    origin[0] = origin[1] = origin[2] = 0;
    region[0] = imgWidth;
    region[1] = imgHeight;
    region[2] = 1;
    unsigned char *yuvData = new unsigned char[4 * imgWidth * imgHeight];

    // fill the data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < imgWidth; x += 4) {
            yuvData[y * width + x]     = input;
            yuvData[y * width + x + 1] = input;
            yuvData[y * width + x + 2] = input;
            yuvData[y * width + x + 3] = input;
        }
    }
    for (int y = height; y < imgHeight; y++) {
        for (int x = 0; x < imgWidth; x += 4) {
            yuvData[y * width + x]     = inputUV;
            yuvData[y * width + x + 1] = inputUV;
            yuvData[y * width + x + 2] = inputUV;
            yuvData[y * width + x + 3] = inputUV;
        }
    }

    trace("writing yuv image...");
    cl::Event ev0;
    m_queue->enqueueWriteImage(yuv, CL_TRUE, origin, region, 0, 0, yuvData, 0, &ev0);
    recordEvent("write_yuv_image", std::pair<cl::Event, cl::Event>(ev0, ev0));
//
//    kCall("img_h4", cl::NDRange(imgWidth, height), yuv, yuvd, yuv);
//
//    cl::Event ev;
//    m_queue->enqueueReadImage(yuv, CL_TRUE, origin, region, 0, 0, yuvData, 0, &ev);
//    recordEvent("read_yuv_out_image", std::pair<cl::Event, cl::Event>(ev, ev));
//

//    trace("validating image...");
//    float inY = (float)input;
//    float outY = inY / 10.f;
//    int output = round(outY);
//    int outputUV = round(((float)inputUV) / 20.f);
//    trace("input = %d, output = %d", input, output);
//    trace("inputUV = %d, outputUV = %d", inputUV, outputUV);

//    int err = 0;
//    for (int y = 0; y < height; y++) {
//        for (int x = 0; x < width; x += 4) {
//            if (yuvData[y * width + x] != output) { err++; trace("y:%d x:%d >> %d  != %d\n", y, x, output, yuvData[y * width + x]); }
//            if (yuvData[y * width + x + 1] != output) { trace("y:%d x:%d >> %d != %d\n", y, x + 1, output, yuvData[y * width + x + 1]); }
//            if (yuvData[y * width + x + 2] != output) { trace("y:%d x:%d >> %d != %d\n", y, x + 2, output, yuvData[y * width + x + 2]); }
//            if (yuvData[y * width + x + 3] != output) { trace("y:%d x:%d >> %d != %d\n", y, x + 3, output, yuvData[y * width + x + 3]); }
//        }
//    }
//    for (int y = height; y < imgHeight; y++) {
//        for (int x = 0; x < width; x += 4) {
//            if (yuvData[y * width + x] != outputUV) { err++; trace("UV  y:%d x:%d >> %d  != %d\n", y, x, outputUV, yuvData[y * width + x]); }
//            if (yuvData[y * width + x + 1] != outputUV) { trace("UV  y:%d x:%d >> %d != %d\n", y, x + 1, outputUV, yuvData[y * width + x + 1]); }
//            if (yuvData[y * width + x + 2] != outputUV) { trace("UV  y:%d x:%d >> %d != %d\n", y, x + 2, outputUV, yuvData[y * width + x + 2]); }
//            if (yuvData[y * width + x + 3] != outputUV) { trace("UV  y:%d x:%d >> %d != %d\n", y, x + 3, outputUV, yuvData[y * width + x + 3]); }
//        }
//    }
//    trace("err == %d", err);
//    return;

    trace("processing image...");
    timestamp("image_half8(A,B,A)");
    for(int i = 0; i < 100; i++) {
        kCall("img_h8", cl::NDRange(width / 4, height / 2), yuv, yuvd, yuv_out);
    }
    m_queue->finish();
    looper(m_appData);
    timestamp("image_half8(A,B,A)");
    trace("done processing image");

    timestamp("image_half8(A,B,C)");
    for(int i = 0; i < 100; i++) {
        kCall("img_h8", cl::NDRange(width / 4, height / 2), yuv, yuvd, yuv_out);
    }
    m_queue->finish();
    looper(m_appData);
    timestamp("image_half8(A,B,C)");

    timestamp("image_half4(A,B,C)");
    for(int i = 0; i < 100; i++) {
        kCall("img_h4", cl::NDRange(width / 4, height), yuv, yuvd, yuv_out);
    }
    m_queue->finish();
    looper(m_appData);
    //printEventTimings();
    timestamp("image_half4(A,B,C)");

    timestamp("image_half4(A,B,A)");
    for(int i = 0; i < 100; i++) {
        kCall("img_h4", cl::NDRange(width / 4, height), yuv, yuvd, yuv);
    }
    m_queue->finish();
    looper(m_appData);
    //printEventTimings();
    timestamp("image_half4(A,B,A)");
//
//    timestamp("buffer_half8(A,A)");
//    for(int i = 0; i < 100; i++) {
//        kCall("buf_h8", cl::NDRange(width / 4 , height / 2), in1, in2, out);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    //printEventTimings();
//    timestamp("buffer_half8(A,A)");
//    timestamp("buffer_half8(A,B)");
//    for(int i = 0; i < 100; i++) {
//        kCall("buf_h8", cl::NDRange(width / 8 , height), in1, in2, in1);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    //printEventTimings();
//    timestamp("buffer_half8(A,B)");
//
//
//    timestamp("image_float8(A,B,A)");
//    for(int i = 0; i < 100; i++) {
//        kCall("img_f8", cl::NDRange(width / 8, height), yuv, yuvd, yuv);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    timestamp("image_float8(A,B,A)");
//
//    timestamp("image_float8(A,B,C)");
//    for(int i = 0; i < 100; i++) {
//        kCall("img_f8", cl::NDRange(width / 4, height / 2), yuv, yuvd, yuv);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    timestamp("image_float8(A,B,C)");
//
//    timestamp("image_float4(A,B,A)");
//    for(int i = 0; i < 100; i++) {
//        kCall("img_f4", cl::NDRange(width / 4, height), yuv, yuvd, yuv);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    timestamp("image_float4(A,B,A)");
//
//    timestamp("image_float4(A,B,C)");
//    for(int i = 0; i < 100; i++) {
//        kCall("img_f4", cl::NDRange(width / 4, height), yuv, yuvd, yuv_out);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    timestamp("image_float4(A,B,C)");
//

//    timestamp("buffer_half8(A,A)");
//    for(int i = 0; i < 100; i++) {
//        kCall("buf_h8", cl::NDRange(width / 8 , height), in1, in2, out);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    //printEventTimings();
//    timestamp("buffer_half8(A,A)");
//    timestamp("buffer_half8(A,B)");
//    for(int i = 0; i < 100; i++) {
//        kCall("buf_h8", cl::NDRange(width / 8 , height), in1, in2, in1);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    //printEventTimings();
//    timestamp("buffer_half8(A,B)");

//
//    timestamp("buffer_float8(A,A)");
//    for(int i = 0; i < 100; i++) {
//        kCall("buf_f8", cl::NDRange(width / 8 , height), in1, in2, out);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    //printEventTimings();
//    timestamp("buffer_float8(A,A)");
//    timestamp("buffer_float8(A,B)");
//    for(int i = 0; i < 100; i++) {
//        kCall("buf_f8", cl::NDRange(width / 8 , height), in1, in2, in1);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    //printEventTimings();
//    timestamp("buffer_float8(A,B)");


//    timestamp("image_float8(A,B,A)");
//    for(int i = 0; i < 100; i++) {
//        kCall("img_f8", cl::NDRange(width / 8 , height), yuv, yuvd, yuv);
//    }
//    m_queue->finish();
//    looper(m_appData);
//    //printEventTimings();
//    timestamp("image_float8(A,B,A)");
//
//    timestamp("image_float8(A,B,C)");
//    for(int i = 0; i < 100; i++) {
//        kCall("img_f8", cl::NDRange(width / 8 , height), yuv, yuvd, yuv_out);
//        m_queue->finish();
//    }
//    m_queue->finish();
//    looper(m_appData);
//    //printEventTimings();
//    timestamp("image_float8(A,B,C)");
}

void* Pipeline::pixelMix(unsigned char* yuv, unsigned char *yuv_den, unsigned char* yuv_out) {
    // map input
    cl::size_t<3> origin, region;
    origin[0] = origin[1] = origin[2] = 0;
    region[0] = m_width / 4;
    region[1] = m_height;
    region[2] = 1;
    trace("write yuv image");
    cl::Event ev0;
    m_queue->enqueueWriteImage(*m_yImage, CL_TRUE, origin, region, 0, 0, yuv, 0, &ev0);
    recordEvent("write_yuv_image", std::pair<cl::Event, cl::Event>(ev0, ev0));

    trace("write yuv_den image");
    cl::Event ev1;
    m_queue->enqueueWriteImage(*m_ydImage, CL_TRUE, origin, region, 0, 0, yuv_den, 0, &ev1);
    recordEvent("write_yuv_den_image", std::pair<cl::Event, cl::Event>(ev1, ev1));

    // process
    kCall("pixelMix_image_half4_nodesat_image", cl::NDRange(m_width / 8, m_height), *m_yImage, *m_uvImage, *m_ydImage, *m_yuvOutImage);
    kCall("pixelMix_image_float4_nodesat_image", cl::NDRange(m_width / 4, m_height), *m_yImage, *m_uvImage, *m_ydImage, *m_yuvOutImage);
    kCall("pixelMix_image_float4_nodesat", cl::NDRange(m_width / 4, m_height), *m_yImage, *m_uvImage, *m_ydImage, *m_yImage, *m_uvImage);
//    m_queue->finish();
    trace("read yuv_out image");
    cl::Event ev2;
    m_queue->enqueueReadImage(*m_yuvOutImage, CL_TRUE, origin, region, 0, 0, yuv_out, 0, &ev2);
    recordEvent("read_yuv_out_image", std::pair<cl::Event, cl::Event>(ev2, ev2));

    trace("read yuv image");
    cl::Event ev3;
    m_queue->enqueueReadImage(*m_yImage, CL_TRUE, origin, region, 0, 0, yuv, 0, &ev3);
    recordEvent("read_yuv_image", std::pair<cl::Event, cl::Event>(ev3, ev3));

    printEventTimings();

//
//    printEventTimings();
    trace("done with pixelMix: frame size = %d x %d", m_width, m_height);
    return 0;
}


Pipeline::Pipeline() {
    m_context = NULL;
    m_queue = NULL;
    m_rccbBufA = NULL;
    m_rccbBufB = NULL;
    m_srgbImageA = NULL;
    m_srgbImageB = NULL;
    m_rccbImage = NULL;
    m_rccbTemporalImage = NULL;
    m_isContextOwner = false;
    m_isDeviceOwner = false;
    m_isQueueOwner = false;
    m_appData = NULL;
    m_isRunning = false;
}

Pipeline::~Pipeline() {
    if (m_rccbBufA)
        delete m_rccbBufA;
    if (m_rccbBufB)
        delete m_rccbBufB;
    if (m_srgbImageA)
        delete m_srgbImageA;
    if (m_srgbImageB)
        delete m_srgbImageB;
    if (m_rccbImage)
        delete m_rccbImage;
    if (m_rccbTemporalImage)
        delete m_rccbTemporalImage;
    if (m_queue && m_isQueueOwner)
        delete m_queue;
    if (m_context && m_isContextOwner)
        delete m_context;
}

bool Pipeline::init(int width, int height, int type, int depth, const string& path, RCCB *rccb,
        void* appData, cl::Context *context, cl::Device *device, cl::CommandQueue *queue) {

    if (DEBUG) trace("width:%d, height:%d, depth:%d", width, height, depth);

    std::string fullPath = path;
#ifdef NOANDROID
    if (!fullPath.empty() && *fullPath.rbegin() != '/') {
        if (DEBUG) trace("path: %s", path.c_str());
        char *prefix = getcwd(NULL, 256);
        if (DEBUG) trace("prefix: ");
        if (DEBUG) trace("%s", prefix);
        fullPath = prefix;
        fullPath += "/" + path;
        if (DEBUG) trace("%s", fullPath.c_str());
    }
#endif
    m_path = fullPath;
    m_rccb = rccb;
    m_width = width;
    m_height = height;
    m_type = type;
    m_depth = depth;
    m_appData = appData;

    if ((width % 8) || (height % 2)) {
        trace("Pipeline: image dimensions should be divisible by 8x2");
        return false;
    }

    if (!initCL(context, device, queue)) {
        trace("Pipeline: failed to initialize CL");
        return false;
    }

    adjustCC();

    generateRccbHeader(m_rccb, path);

    if (!initKernels(path)) {
        trace("Pipeline: failed to initialize kernels");
        return false;
    }

    if (!initBuffers(width, height)) {
        trace("Pipeline: failed to initialize buffers");
        return false;
    }

    return true;
}

struct AsyncTask {
    Pipeline *m_pipe;
    cl::Event *m_evt;
    void *m_buf;
    cl::Memory *m_mem;
    bool m_done;
    bool (*m_handler)(void *, void*);
    cl::size_t<3> origin, region;
    size_t image_row_pitch, image_slice_pitch;
    bool isWriter;

    AsyncTask(Pipeline *pipe, cl::Memory *mem, bool (*handler)(void *, void*)) {
        m_pipe = pipe;
        m_mem = mem;
        m_buf = NULL;
        m_handler = handler;
        m_done = true;
        m_evt = new cl::Event();
        origin[0] = origin[1] = origin[2] = 0;
        region[0] = m_pipe->m_width;
        region[1] = m_pipe->m_height;
        region[2] = 1;
    }

    ~AsyncTask() {
        if (m_evt)
            delete m_evt;
    }

    bool startRead() {
        isWriter = false;
        if (DEBUG) trace("MAIN:%p     AsyncTask: startRead", this);
        if (m_done) {
            m_done = false;
            int size = m_pipe->m_width * m_pipe->m_height * sizeof(cl_short);
            cl::Buffer *input = (cl::Buffer *) m_mem;
            if (DEBUG) trace("MAIN:%p     AsyncTask: startRead, mem =  %p", this, input);
            m_buf = m_pipe->m_queue->enqueueMapBuffer(*input, CL_FALSE, CL_MAP_WRITE, 0, size, 0,
                    m_evt);
            if (DEBUG) trace("MAIN:%p     AsyncTask: setCallback: buf = %p", this, m_buf);
            m_evt->setCallback(CL_COMPLETE, &callback, (void*) this);
        }
        if (DEBUG) trace("MAIN:%p     AsyncTask: startRead.end , done = %d", this, m_done);
        return !m_done;
    }

    bool startWrite() {
        isWriter = true;
        if (DEBUG) trace("MAIN:%p     AsyncTask: startWrite, done =  %d", this, m_done);
        if (m_done) {
            m_done = false;
            cl::Image2D *img = (cl::Image2D *) m_mem;
            if (DEBUG) trace("MAIN:%p     AsyncTask: startWrite, mem =  %p", this, img);
            m_buf = m_pipe->m_queue->enqueueMapImage(*img, CL_TRUE, CL_MAP_READ, origin, region,
                    &image_row_pitch, &image_slice_pitch, 0, m_evt);
            if (DEBUG) trace("MAIN:%p     AsyncTask: startWrite, setCallback: buf = %p", this, m_buf);
            m_evt->setCallback(CL_COMPLETE, &callback, (void*) this);
        }
        if (DEBUG) trace("MAIN:%p     AsyncTask: startWrite.end , done = %d", this, m_done);
        return !m_done;
    }

    bool wait(unsigned long tick = WAIT_TICK, unsigned long timeout = WAIT_TICK * 10000) {
        if (DEBUG) trace("MAIN:%p     AsyncTask: wait enter : done == %d", this, m_done);
        unsigned long timeToWait = timeout;
        while (timeToWait > 0) {
            if (m_done) {
                if (m_buf != NULL) {
                    m_pipe->m_queue->enqueueUnmapMemObject(*m_mem, m_buf);
                }
                if (DEBUG) trace("MAIN:%p     AsyncTask: completed", this);
                return true;
            }
            if (DEBUG);
                //trace("MAIN:%p     AsyncTask: sleep %s", this, isWriter ? " while writing":" while reading");
            usleep(tick);
            timeToWait -= tick;
        }
        return false;
    }

private:
    void handle(cl_event evt, cl_int status) {
        if (DEBUG) trace("%p     AsyncTask: handle, done = %d", this, m_done);
        //timestamp("handle");

        if (!m_handler(m_buf, m_pipe->m_appData)) {
            if (DEBUG) trace("%p     AsyncTask: handler requests to stop", this);
            m_pipe->stop();
        }
        m_done = true;
        if (DEBUG) trace("%p     AsyncTask: handle.end done = %d, mem = %p", this, m_done, m_mem);
        //timestamp("handle");
    }

    static void CL_CALLBACK callback(cl_event evt, cl_int status, void *self) {
        if (DEBUG) trace("CL_CALLBACK callback");
        ((AsyncTask *)self)->handle(evt, status);
    }
};

bool Pipeline::start(bool (*read)(void *, void*), bool (*write)(void *, void*), bool (*looper)(void *)) {
#if 0
    for(int i = 0; i < 100; i++) {
        looper(m_appData);
        timestamp("floats");
        floats(m_width, m_height, looper);
        m_queue->finish();
        printEventTimings();
        timestamp("floats");
    }
    return;
#endif

    if (DEBUG) trace("start: running == %d", m_isRunning);
    if (m_isRunning)
        return false;

    m_isRunning = true;

    AsyncTask *input = new AsyncTask(this, m_rccbBufA, read);
    AsyncTask *nextInput = new AsyncTask(this, m_rccbBufB, read);
    AsyncTask *output = new AsyncTask(this, m_srgbImageA, write);
    AsyncTask *nextOutput = new AsyncTask(this, m_srgbImageA, write);

    if (DEBUG) trace("MAIN:input->startRead()");
    if (!input->startRead())
        FATAL("Error: Wrong initial read state");

    int T = 0;
    for (long i = 0; m_isRunning; i++) {
        n_frames++;
        if (i % 100 == 0) timestamp("process 100 frames");
        if (DEBUG) trace("frame%d/%d: %p, %p, %p, %p", i, m_rccb->nFrames, input, nextInput, output, nextOutput);
        cl_int4 bb;
        cl_float4 wg;
        bb.s[0] = m_rccb->bb[1];
        bb.s[1] = m_rccb->bb[0];
        bb.s[2] = m_rccb->bb[3];
        bb.s[3] = m_rccb->bb[2];
        wg.s[0] = m_rccb->wg[R];
        wg.s[1] = wg.s[2] = m_rccb->wg[G];
        wg.s[3] = m_rccb->wg[B];
        cl_float bt = m_rccb->bt * (1 << m_depth);
//        if (i >= m_rccb->nFrames) {
//            T = (T + 2) % 10;
//            i = 0;
//        }
        T = m_rccb->nAggregate + 1;
        cl_int tcnt = i;
        if (tcnt > T) {
            tcnt = T;
        }

        if (DEBUG) trace("MAIN:nextInput->startRead():%p", nextInput);
        // Submit async request to read next frame
        if (!nextInput->startRead())
            FATAL("Error: Wrong read state!");

        // Let main thread go
        looper(m_appData);

        if (DEBUG) trace("MAIN:input->wait():%p", input);
        // Wait until previous request completed
        if (!input->wait())
            FATAL("Error: Frame read timeout!");

        if (!m_isRunning) {
            trace("No more frames available. Terminating pipeline ...");
            break;
        }

        // Process input
        cl::Buffer *in = (cl::Buffer *) (input->m_mem);
        if (DEBUG) trace("MAIN:packAdjustLevels: in == %p", in);
        kCall("packAdjustLevels", cl::NDRange(m_width / 4, m_height / 2), *in, *m_rccbImage, bb, wg,
                bt, *m_rccbTemporalImage, tcnt);

        // Wait until previous request to write is completed
        if (DEBUG) trace("MAIN:output->wait():%p", output);
        if (!output->wait())
            FATAL("Error: output image write timeout!");

        // Generate output
        cl::Image2D *out = (cl::Image2D *) (output->m_mem);
        if (DEBUG) trace("MAIN:kCall rccb, out = %p", out);
        kCall("rccb", cl::NDRange(m_width / 2, m_height / 2), *m_rccbImage, *out);

        // Submit async request to write image
        if (DEBUG) trace("MAIN:output->startWrite():%p", nextOutput);
        if (!nextOutput->startWrite())
            FATAL("Error: Wrong write state!");

        //if (TRACE_TIME) printEventTimings();

        swap(input, nextInput);
        swap(output, nextOutput);
        swap(m_rccbImage, m_rccbTemporalImage);
        //timestamp("process frame");
    }

    // Wait until all previous request completed
    if (!nextInput->wait())
        FATAL("Error: Frame read timeout!");
    if (!output->wait())
        FATAL("Error: output image write timeout!");

    delete input;
    delete nextInput;
    delete output;
    delete nextOutput;

    return true;
}

bool Pipeline::stop() {
    if (!m_isRunning)
        return false;
    m_isRunning = false;
    return true;
}

void* Pipeline::processFrame(short *inImg, void *outImg) {
    if (outImg != NULL) {
        if (DEBUG) trace("unmap");
        cl::Event ev;
        m_queue->enqueueUnmapMemObject(*m_srgbImageA, outImg, 0, &ev);
        recordEvent("unmapBuffer", std::pair<cl::Event, cl::Event>(ev, ev));
    }

    if (inImg == NULL) {
        return NULL;
    }

    {
        if (DEBUG) trace("mapInputBuffer");
        cl::Event ev0;
        int size = sizeof(short) * m_width * m_height;
        //        m_queue->enqueueWriteBuffer(*m_rccbBuf, 0, 0, size, &inImg[m_rccb->type == IT_CRBC], 0, &ev0);
        void *buf = m_queue->enqueueMapBuffer(*m_rccbBufA, CL_TRUE, CL_MAP_WRITE, 0, size, 0, &ev0);
        recordEvent("mapInputBuffer", std::pair<cl::Event, cl::Event>(ev0, ev0));
        //        ev0.setCallback(CL_COMPLETE, &notify_callback, (void*)"mapInputBuffer");

        // TODO: unpack
        timestamp("copy buffer");
        memcpy(buf, &inImg[m_rccb->type == IT_CRBC], size);
        timestamp("copy buffer");

        cl::Event ev;
        m_queue->enqueueUnmapMemObject(*m_rccbBufA, buf, 0, &ev);
        recordEvent("unmapInputBuffer", std::pair<cl::Event, cl::Event>(ev, ev));
        //        ev.setCallback(CL_COMPLETE, &read, buf);
    }

    cl::Kernel k;
    cl_int4 bb;
    cl_float4 wg;
    bb.s[0] = m_rccb->bb[1];
    bb.s[1] = m_rccb->bb[0];
    bb.s[2] = m_rccb->bb[3];
    bb.s[3] = m_rccb->bb[2];

    wg.s[0] = m_rccb->wg[R];
    wg.s[1] = wg.s[2] = m_rccb->wg[G];
    wg.s[3] = m_rccb->wg[B];
    cl_float bt = m_rccb->bt * (1 << m_depth);

    if (DEBUG) trace("packAdjustLevels");
    getKernel("packAdjustLevels", k);
    k.setArg(0, *m_rccbBufA);
    k.setArg(1, *m_rccbImage);
    k.setArg(2, bb);
    k.setArg(3, wg);
    k.setArg(4, bt);
    cl::Event ev1;
    m_queue->enqueueNDRangeKernel(k, cl::NullRange, cl::NDRange(m_width / 4, m_height / 2),
            cl::NullRange, 0, &ev1);
    recordEvent("packAdjustLevels", std::pair<cl::Event, cl::Event>(ev1, ev1));

    if (DEBUG) trace("rccb");
    getKernel("rccb", k);
    k.setArg(0, *m_rccbImage);
    k.setArg(1, *m_srgbImageA);
    cl::Event ev2;
    m_queue->enqueueNDRangeKernel(k, cl::NullRange, cl::NDRange(m_width / 2, m_height / 2),
            cl::NullRange, 0, &ev2);
    recordEvent("rccb", std::pair<cl::Event, cl::Event>(ev2, ev2));

    if (DEBUG) trace("read buffer");
    cl::Event ev3;
    cl::size_t<3> origin, region;
    origin[0] = origin[1] = origin[2] = 0;
    region[0] = m_width;
    region[1] = m_height;
    region[2] = 1;
    size_t image_row_pitch, image_slice_pitch;
    char *out = (char *) m_queue->enqueueMapImage(*m_srgbImageA, CL_TRUE, CL_MAP_READ, origin,
            region, &image_row_pitch, &image_slice_pitch, 0, &ev3);
    if (DEBUG) trace("done");
    recordEvent("mapImage", std::pair<cl::Event, cl::Event>(ev3, ev3));
    if (DEBUG) trace("image_row_pitch = %d, image_slice_pitch = %d", image_row_pitch, image_slice_pitch);
    if (TRACE_TIME)
        printEventTimings();

    return out;
}

bool Pipeline::initCL(cl::Context *context, cl::Device *device, cl::CommandQueue *queue) {
    if (context) {
        m_context = context;
    } else {
        trace("initCL: create context");
        m_context = new cl::Context(CL_DEVICE_TYPE_GPU, 0, handleError);
        if (!m_context) {
            trace("Pipeline: failed to create CL context");
            return false;
        }
        m_isContextOwner = true;
    }

    if (device) {
        m_device = *device;
    } else {
        if (DEBUG) trace("initCL: create device");
        vector < cl::Device > devices = m_context->getInfo<CL_CONTEXT_DEVICES>();
        if (devices.size() == 0) {
            trace("Pipeline: no cl devices in context");
            return false;
        }
        m_device = devices[0];
    }

    if (queue) {
        m_queue = queue;
    } else {
        if (DEBUG) trace("initCL: create queue");
        cl_command_queue_properties queueProps = 0;
        queueProps |= CL_QUEUE_PROFILING_ENABLE;
        m_queue = new cl::CommandQueue(*m_context, m_device, queueProps);
        if (!m_queue) {
            trace("Pipeline: failed to create queue");
            return false;
        }
        m_isQueueOwner = true;
    }

    return true;
}

bool Pipeline::generateRccbHeader(RCCB *rccb, const std::string &path) {
    std::ofstream def;
    std::string fname = path + "/rccb.h";
    def.open(fname.c_str());
    def << "// Auto-generated file"
            "\n#ifndef RCCB_H_\n#define RCCB_H_" << "\n#define IMAGE_WIDTH " << m_width
            << "\n#define IMAGE_HEIGHT " << m_height << "\n#define IMAGE_DEPTH " << m_depth;
    float fr = m_rccb->pf_mix[0];
    float fb = m_rccb->pf_mix[0];
    float fc = 1.0 - fb - fr;
    def << "\n#define N_SAMPLES 2";
    def << "\n#define PF_MIX_R " << fr << (fr != 0.f ? "f" : ".f");
    def << "\n#define PF_MIX_C " << fc << (fc != 0.f ? "f" : ".f");
    def << "\n#define PF_MIX_B " << fb << (fb != 0.f ? "f" : ".f");

#define DEF(val) def << "\n#define RCCB_" << #val << " " << rccb->val;
    DEF(fd)
    // TODO: add more rccb params
#undef DEF

    // Color correction
    for (int i = 0; i < 9; i++) {
        def << "\n#define RCCB_cc" << i << " " << rccb->cc[i] << ((rccb->cc[i] - floor(rccb->cc[i]))!= 0.f ? "f" : ".f");
    }

    def << "\n#endif // RCCB_H_";
    def.close();
    return true;
}

bool Pipeline::initKernels(const string& path) {
    if (DEBUG) trace("initKernels");
    timestamp("initKernels");
    std::string prefix = path;
    if (!prefix.empty() && *prefix.rbegin() != '/') {
        prefix += "/";
    }
    const char* const clFiles[] = { "packing.cl", "rccb.cl", "point.cl", "halfs.cl", "floats.cl" };
    const int nCLFiles = sizeof(clFiles) / sizeof(clFiles[0]);
    for (int i = 0; i < nCLFiles; ++i) {
        if (loadKernelsFile(prefix + clFiles[i]) != CL_SUCCESS) {
            trace("failed to load kernel file %s", clFiles[i]);
            return false;
        }
    }

    timestamp("initKernels");
    return true;
}

cl_int Pipeline::loadKernelsFile(const string &fn) {
    ifstream input(fn.c_str());
    if (!input) {
        trace("Pipeline: failed to load cl source file %s", fn.c_str());
        return CL_INVALID_PROGRAM;
    }
    string source((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
    return loadKernelsString(source, fn);
}

cl_int Pipeline::loadKernelsString(const string &source, const string &tag) {
    cl::Program::Sources _source(1, std::make_pair(source.c_str(),strlen(source.c_str())));
    cl::Program program(*m_context, _source);
    cl_int err;
#ifdef __CL_ENABLE_EXCEPTIONS
    try {
        buildProgram( program );
        err = CL_SUCCESS;
    } catch( cl::Error &error ) {
        err = error.err();
    }
#else
    err = buildProgram(program);
#endif
    if (err != CL_SUCCESS)
        trace("error %d building %s", err, tag.c_str());

    //output log, kernels and per-kernel information
    trace("Build log for: %s", tag.c_str());
    trace("%s", program.getBuildInfo < CL_PROGRAM_BUILD_LOG > (m_device).c_str());

    vector < cl::Kernel > kernels;
    program.createKernels(&kernels);

    for (::size_t i = 0; i < kernels.size(); ++i) {
        cl::Kernel kernel = kernels[i];
        string kernelName = kernel.getInfo<CL_KERNEL_FUNCTION_NAME>();
        if (m_kernels.count(kernelName))
            trace("Kernel %s is being overwritten!", kernelName.c_str());
        m_kernels[kernelName] = kernel;

        //output some useful info for each kernel
        trace("   %s  mws:%d  lmem:%d  pmem:%d", kernelName.c_str(),
                kernel.getWorkGroupInfo < CL_KERNEL_WORK_GROUP_SIZE > (m_device),
                kernel.getWorkGroupInfo < CL_KERNEL_LOCAL_MEM_SIZE > (m_device),
                kernel.getWorkGroupInfo < CL_KERNEL_PRIVATE_MEM_SIZE > (m_device));

    }
    trace("-----------------------------------------------------");

    return err;
}

cl_int Pipeline::buildProgram(cl::Program &program) {
    vector < cl::Device > devices;
    devices.push_back(m_device);
#if __APPLE__
    std::string buildParams = "-I" + m_path + " -DFP_16 -cl-fast-relaxed-math -cl-std=CL1.1";
#elif
    std::string buildParams = "-I" + m_path + " -qcom-accelerate-16-bit -cl-fast-relaxed-math -O3";
#endif
    return program.build(devices, buildParams.c_str());
}

bool Pipeline::initBuffers(int width, int height) {
    timestamp("initBuffers");
    m_rccbBufA = new cl::Buffer(*m_context, CL_MEM_READ_WRITE, sizeof(cl_short) * width * height);
    m_rccbBufB = new cl::Buffer(*m_context, CL_MEM_READ_WRITE, sizeof(cl_short) * width * height);

    cl::ImageFormat format = cl::ImageFormat(CL_RGBA, CL_UNORM_INT8);
    m_rccbImage = new cl::Image2D(*m_context, CL_MEM_READ_WRITE, format, width / 2, height / 2);
    m_rccbTemporalImage = new cl::Image2D(*m_context, CL_MEM_READ_WRITE, format, width / 2, height / 2);
    m_srgbImageA = new cl::Image2D(*m_context, CL_MEM_READ_WRITE, format, width, height);
    m_srgbImageB = new cl::Image2D(*m_context, CL_MEM_READ_WRITE, format, width, height);

    m_yImage = new cl::Image2D(*m_context, CL_MEM_READ_WRITE, format, width / 4, height);
    m_ydImage = new cl::Image2D(*m_context, CL_MEM_READ_WRITE, format, width / 4, height);
    m_uvImage = new cl::Image2D(*m_context, CL_MEM_READ_WRITE, format, width / 8, height / 2);
    m_yuvOutImage = new cl::Image2D(*m_context, CL_MEM_READ_WRITE, format, width / 4, height * 3/2);

    timestamp("initBuffers");
    return true;
}

/**
 * Static callback from the opencl context
 */
void Pipeline::handleError(const char* errorinfo, const void* private_info_size, ::size_t cb,
        void* user_data) {
    trace("Pipeline error: %s", errorinfo);
}

void Pipeline::clearEvents() {
    m_events.clear();
}

void Pipeline::recordEvent(const std::string& name, std::pair<cl::Event, cl::Event> ev) {
    m_events[name] = ev;
}

void Pipeline::printEventTimings() {
    trace("-----------------------------------------------------------");
    trace("Kernel                         Time(ms)");
    trace("-----------------------------------------------------------");
    float total = 0.0f;
    for (map<string, std::pair<cl::Event, cl::Event> >::iterator it = m_events.begin();
            it != m_events.end(); ++it) {
        cl::Event &ev_start = it->second.first;
        cl::Event &ev_end = it->second.second;
        int64_t start = ev_start.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        int64_t end = ev_end.getProfilingInfo<CL_PROFILING_COMMAND_END>();
        float dt = (end - start) * 1.0e-6;
        trace("%-32s %f ms", it->first.c_str(), dt);
        total += dt;
    }
    trace("------------------------------------------------------------");
    trace("time:" "%-32s %f ms", "Total frame time", total);
    trace("------------------------------------------------------------");
}

void Pipeline::currentTime(struct timespec &ts) {
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
}

int64_t Pipeline::currentTime() {
    struct timespec ts;
    currentTime(ts);
    return ts.tv_sec * int64_t(1000000) + ts.tv_nsec / 1000;
}

void Pipeline::incTimeSpec(struct timespec &ts, int64_t dt) {
    int ds = dt / 1000000;
    int dns = (dt - ds * 1000000) * 1000;
    ts.tv_sec += ds;
    ts.tv_nsec += dns;

    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    } else if (ts.tv_nsec < 0) {
        ts.tv_sec -= 1;
        ts.tv_nsec += 1000000000;
    }
}

bool Pipeline::getKernel(const std::string name, cl::Kernel &k) {
    map<string, cl::Kernel>::iterator it = m_kernels.find(name);
    if (it == m_kernels.end()) {
        trace("Pipeline error: no kernel named %s", name.c_str());
        return false;
    }
    k = it->second;
    return true;
}

inline short Pipeline::getPixel(const short * img, int x, int y) {
    if (x < 0 || y < 0 || (unsigned) x >= m_width || (unsigned) y >= m_height) {
        trace("Out of boundaries error! x:%d, y:%d", x, y);
        return 0;
    }
    return img[y * m_width + x];
}

inline void Pipeline::setPixel(short * img, int x, int y, short pixel) {
    if (x < 0 || y < 0 || (unsigned) x >= m_width || (unsigned) y >= m_height) {
        trace("Out of boundaries error! x:%d, y:%d", x, y);
        return;
    }
    img[y * m_width + x] = pixel;
}

static int median25(int p[25]) {
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

static void median5(short *in, short *out, int width, int height) {
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int p[25];
            int x0, y0;
            for (y0 = -2; y0 < 3; y0++) {
                for (x0 = -2; x0 < 3; x0++) {
                    if ((y + y0 >= 0) && (y + y0 < height) && (x + x0 >= 0) && (x + x0 < width)) {
                        p[((y0 + 2) * 5) + (x0 + 2)] = in[(y + y0) * width + x + x0];
                    } else {
                        p[((y0 + 2) * 5) + (x0 + 2)] = -1;
                    }
                }
            }

            for (x0 = 1; x0 < 25; x0 += 2) {
                p[x0] = -1;
            }
            p[6] = -1;
            p[8] = -1;
            p[16] = -1;
            p[18] = -1;
            out[y * width + x] = median25(p);
        }
    }
}

bool Pipeline::autoBlackBalance(short *img) {
    timestamp("autoBlackBalance");

    int hist[1 << 14] = { 0 };

    // 1. median 5x5 filter
    short *med = new short[m_width * m_height];
    median5(img, med, m_width, m_height);

    // 2. histogram
    int mask = (1 << m_depth) - 1;
    for (unsigned int y = 0; y < m_height; y += 2) {
        for (unsigned int x = 0; x < m_width; x += 2) {
            short pp = getPixel(med, x, y);
            hist[pp & mask]++;
        }
    }

    // 3. finding threshold
    int ts = m_height * m_width / 4 * m_rccb->ba;
    int mx = (1 << m_depth) * m_rccb->bam;

    int s = 0;
    int x = 0;
    for (x = 0; x < mx && s < ts; x++) {
        s += hist[x];
        if (s >= ts) {
            trace("s >= tx: %d > %d", s, ts);
        }
        if (x >= mx) {
            trace("x >= mx: %d > %d", x, mx);
        }
    }

    // 4. computing average
    int bt = x;
    int sum[4] = { 0 };
    int cnt = 0;
    for (unsigned y = 0; y < m_height; y += 2) {
        for (unsigned x = 0; x < m_width; x += 2) {
            short pix = getPixel(img, x, y);
            if (pix < bt) {
                sum[0] += getPixel(med, x, y);
                sum[1] += getPixel(med, x + 1, y);
                sum[2] += getPixel(med, x, y + 1);
                sum[3] += getPixel(med, x + 1, y + 1);
                cnt++;
            }
        }
    }
    if (cnt) {
        for (int i = 0; i < 4; i++)
            m_rccb->bb[i] = sum[i] / cnt;
    }

    delete med;
    timestamp("autoBlackBalance");
    return cnt > 0;
}

bool Pipeline::blackBalance(short *img) {
//    timestamp("\tblackBalance");

    if (m_rccb->ba) {
        autoBlackBalance(img);
    } else if (m_rccb->bw[0] < m_rccb->bw[2] && m_rccb->bw[2] < m_width
            && m_rccb->bw[1] < m_rccb->bw[3] && m_rccb->bw[3] < m_height) {
        double sum[4] = { 0 };
        int cnt[4] = { 0 };
        if (DEBUG) trace("suming area (%d,%d)~(%d,%d", m_rccb->bw[0], m_rccb->bw[1], m_rccb->bw[2],
                m_rccb->bw[3]);
        for (unsigned int y = m_rccb->bw[1]; y < m_rccb->bw[3]; y++)
            for (unsigned int x = m_rccb->bw[0]; x < m_rccb->bw[2]; x++) {
                int i = (y & 1) * 2 + (x & 1);
                sum[i] += getPixel(img, x, y);
                cnt[i]++;
            }
        for (int x = 0; x < 4; x++) {
            m_rccb->bb[x] = sum[x] / cnt[x];
        }
    } else {
        if (m_rccb->bt_defined) {
            if (DEBUG) trace("Cannot use manual black balance -bm with -bt!");
            return false;
        }
        m_rccb->bt = 0;
    }

    if (DEBUG) trace("black balance -bm %d %d %d %d", m_rccb->bb[0], m_rccb->bb[1], m_rccb->bb[2],
            m_rccb->bb[3]);

//    timestamp("\tblackBalance");
    return true;
}

bool Pipeline::whiteBalance(short *img) {
//    timestamp("\twhiteBalance");
    double c2g = 1.;

    if (DEBUG) trace("summing area (%d,%d)~(%d,%d)", m_rccb->ww[0], m_rccb->ww[1], m_rccb->ww[2],
            m_rccb->ww[3]);

    if (m_rccb->ww[0] < m_rccb->ww[2] && m_rccb->ww[2] < m_width && m_rccb->ww[1] < m_rccb->ww[3]
            && m_rccb->ww[3] < m_height) {
        if (DEBUG) trace("white balance stats");
        double sum[4] = { };
        int cnt[4] = { };
        for (unsigned int y = m_rccb->ww[1]; y < m_rccb->ww[3]; y++) {
            for (unsigned int x = m_rccb->ww[0]; x < m_rccb->ww[2]; x++) {
                int i = (y & 1) * 2 + (x & 1);
                sum[i] += getPixel(img, x, y);
                cnt[i]++;
            }
        }
        for (int x = 0; x < 4; x++)
            sum[x] = sum[x] / cnt[x];

        for (int x = 0; x < 4; x++) {
            sum[x] = sum[x] - m_rccb->bb[x];
        }
        if (m_rccb->bayer) {
            double g = (sum[0] + sum[3]) / 2;
            m_rccb->wg[R] = g / sum[1];
            m_rccb->wg[B] = g / sum[2];
            m_rccb->wg[G] = 1.;
        } else {
            m_rccb->wg[R] = MAX(sum[2] / sum[1], 1.0);
            m_rccb->wg[B] = MAX(sum[1] / sum[2], 1.0);
            m_rccb->wg[G] = 1.;
            c2g = (sum[0] + sum[3]) / (m_rccb->wg[R] * sum[1] + m_rccb->wg[B] * sum[2]);
        }
        m_rccb->wg[R] *= c2g;
        m_rccb->wg[B] *= c2g;
    }

    if (DEBUG) trace("white balance -wm %.2f %.2f %.2f  (c2g = %.2f)", m_rccb->wg[R], m_rccb->wg[G],
            m_rccb->wg[B], c2g);

//    timestamp("\twhiteBalance");
    return true;
}

// Adjust color saturation
void Pipeline::adjustCC() {
    const double ry = 0.299, gy = 0.587, by = 0.114;
    for (int i = 0; i < 9; i++) {
        if (DEBUG) trace("cc[%d] = %f", i, m_rccb->cc[i]);
    }
    for (int i = 0; i < 9; i += 3) {
        m_rccb->cc[i + 0] = (m_rccb->cc[i + 0] - ry) * m_rccb->saturation + ry;
        m_rccb->cc[i + 1] = (m_rccb->cc[i + 1] - gy) * m_rccb->saturation + gy;
        m_rccb->cc[i + 2] = (m_rccb->cc[i + 2] - by) * m_rccb->saturation + by;
    }

    for (int i = 0; i < 9; i += 3) {
        m_rccb->cc[i + 1] = m_rccb->cc[i + 1] / (1 - m_rccb->pf_mix[0] - m_rccb->pf_mix[1]);
        m_rccb->cc[i + 0] -= m_rccb->cc[i + 1] * m_rccb->pf_mix[0];
        m_rccb->cc[i + 2] -= m_rccb->cc[i + 1] * m_rccb->pf_mix[1];
    }

    for (int i = 0; i < 9; i++) {
        if (DEBUG) trace("cc[%d] = %f", i, m_rccb->cc[i]);
    }
}
