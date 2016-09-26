#ifndef DEBUG_H
#define DEBUG_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __IHDR_RELEASE_BUILD__
    #define trace(...)
    #define timestamp(label)
    #define hex_dump(data, size)
#else /* Not RELEASE_BUILD */
    #ifdef __func__
    #define trace(...) trace0(__FILE__, __LINE__, __func__, ##__VA_ARGS__)
    #elif defined(__FUNCTION__)
    #define trace(...) trace0(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    #else
    #define trace(...) trace0(__FILE__, __LINE__, "", ##__VA_ARGS__)
    #endif
    long long timestamp(const char* label);
    void trace0(const char* file, int line, const char *func, const char *fmt, ...);
    void hex_dump(void *data, int size);
#endif /* Not RELEASE_BUILD*/
    long long nanotime();

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H */
