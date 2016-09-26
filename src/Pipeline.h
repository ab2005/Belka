/*
 * Pipeline.h
 *
 *  Created on: Mar 25, 2013
 *      Author: ab
 */

#ifndef Pipeline_H_
#define Pipeline_H_

#include <exception>
#include <map>
#include <alloca.h>
#include <deque>
#include <string>
#include <assert.h>

#include <CL/cl.hpp>
// #if defined(__APPLE__) || defined(__MACOSX)
// #include <OpenCL/cl.hpp>
// #else
// #include <CL/cl.hpp>
// #endif

#include "Params.h"

#define FATAL( ... ) fatal_error( __FILE__, __LINE__, __VA_ARGS__ )

//helper template functions for kcall
static inline void kSetArgs(cl::Kernel &kernel, int) {
    //do nothing
}

template<typename T>
inline static void kSetArg(cl::Kernel &kernel, int i, T arg) {
    kernel.setArg(i, arg);
}

template<>
inline void kSetArg(cl::Kernel &kernel, int i, cl::Buffer* arg) {
    kernel.setArg(i, *arg);
}

template<typename Arg0, typename ... Args>
inline void kSetArgs(cl::Kernel &kernel, int i, Arg0 arg0, Args ... args) {
    kSetArg(kernel, i, arg0);
    kSetArgs(kernel, i + 1, args...);
}

class Pipeline {

public:
    Pipeline();
    virtual ~Pipeline();

    bool init(int width, int height, int type, int depth, const std::string& path,
            RCCB *rccb = NULL, void* userData = NULL, cl::Context *context = NULL, cl::Device *device =
                    NULL, cl::CommandQueue *queue = NULL);
    bool blackBalance(short *img);
    bool whiteBalance(short *img);
    void * processFrame(short *in, void *out);
    bool start(bool (*read)(void *, void*), bool (*write)(void *, void*), bool (*looper)(void *));
    bool stop();

    void *pixelMix(unsigned char* yuv, unsigned char* yvu_den, unsigned char* yuv_out);

    cl::CommandQueue *m_queue;
    unsigned int m_width;
    unsigned int m_height;
    int m_type;
    int m_depth;
    // TODO refactor for friends
    void *m_appData;

private:
    void floats(int w, int h, bool (*looper)(void *));

    cl::Context *m_context;
    cl::Device m_device;
    bool m_isContextOwner, m_isDeviceOwner, m_isQueueOwner;
    std::string m_path;
    RCCB *m_rccb;
    bool m_isRunning;

    // Kernels
    std::map<std::string, cl::Kernel> m_kernels;
    // Events
    std::map<std::string, std::pair<cl::Event, cl::Event> > m_events;

    // Buffers
    cl::Buffer *m_rccbBufA, *m_rccbBufB;
    // Images
    cl::Image2D *m_srgbImageA, *m_srgbImageB, *m_rccbImage, *m_rccbTemporalImage;

    // Test Images
    cl::Image2D *m_yImage, *m_uvImage, *m_ydImage, *m_yuvOutImage;

    // Initialization
    bool initCL(cl::Context *context, cl::Device *device, cl::CommandQueue *queue);
    bool generateRccbHeader(RCCB *rccb, const std::string &path);
    bool initKernels(const std::string& path);
    bool initBuffers(int width, int height);
    cl_int buildProgram(cl::Program &program);
    cl_int loadKernelsString(const std::string &source, const std::string& tag);
    cl_int loadKernelsFile(const std::string &fn);
    cl_int loadKernelsBinary(const std::string &fn);
    void adjustCC();

    // Execution
    static inline void fatal_error(const char* file, int line, const char* fmt, ...) {
        printf("%s:%d: ", file, line);
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        printf("\n");
        va_end(args);
        exit(1);
    }

    bool getKernel(const std::string name, cl::Kernel &k);

    template<typename ThreadConfig, typename ... Args>
    void kCall(const std::string& kname, ThreadConfig threadConfig, Args ... args) {
        cl::Kernel kernel;
        if (!getKernel(kname, kernel))
            FATAL("no such kernel %s", kname.c_str());

        //check number of arguments
        int nArgs = kernel.getInfo<CL_KERNEL_NUM_ARGS>();
        int actualNArgs = sizeof...(args);
        if (nArgs != actualNArgs)
            FATAL("kernel %s expected %d arguments, got %d", kname.c_str(), nArgs, actualNArgs);
        kSetArgs(kernel, 0, args...);
        cl::Event ev;
        m_queue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(threadConfig),
                cl::NullRange, 0, &ev);
        recordEvent(kname, std::pair<cl::Event, cl::Event>(ev, ev));
    }

    // Pipeline
    bool autoBlackBalance(short *img);

    // Profiling
    void recordEvent(const std::string &name, std::pair<cl::Event, cl::Event> ev);
    void printEventTimings();
    void clearEvents();
    int64_t currentTime();
    void currentTime(struct timespec &ts);
    void incTimeSpec(struct timespec &ts, int64_t micros);

    // Error handling
    static void handleError(const char* errorinfo, const void* private_info_size, size_t cb,
            void* user_data);
    void handleError(const char* errorinfo, const void* private_info_size, size_t cb);

    // Utils
    short getPixel(const short * img, int x, int y);
    void setPixel(short * img, int x, int y, short pix);
};


#endif /* Pipeline_H_ */
