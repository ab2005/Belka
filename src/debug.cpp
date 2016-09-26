#include <stdarg.h>
#ifndef NOANDROID
#include <android/log.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include "debug.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#define  LOG_TAG        "APTINA"
#define  SOURCE_ROOT    "/jni/"

static void log0(const char* file, int line, const char* func, const char *fmt, va_list vl) {
#ifndef NOANDROID
    char buf[16*1024];
   vsprintf(buf, fmt, vl);
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s", buf);
#else
    vprintf(fmt, vl);
    printf("\n");
#endif
}

void trace0(const char* file, int line, const char* func, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    log0(file, line, func, fmt, vl);
    va_end(vl);
}

void hex_dump(void *data, int size) {
    unsigned char *p = (unsigned char *)data;
    unsigned char c;
    int n;
    char bytestr[4] = { 0 };
    char addrstr[10] = { 0 };
    char hexstr[16 * 3 + 5] = { 0 };
    char charstr[16 * 1 + 5] = { 0 };
    for (n = 1; n <= size; n++) {
        if (n % 16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x", (unsigned int)( p - (unsigned char*) data));
        }

        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr) - strlen(hexstr) - 1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr) - strlen(charstr) - 1);

        if (n % 16 == 0) {
            /* line completed */
            trace("[%4.4s]   %-50.50s  %s", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if (n % 8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr) - strlen(hexstr) - 1);
            strncat(charstr, " ", sizeof(charstr) - strlen(charstr) - 1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        trace("[%4.4s]   %-50.50s  %s", addrstr, hexstr, charstr);
    }
}

static void* hash_map_s2l_create(int capacity);
static int hash_map_s2l_put(void* map, const char* key, long long value);
static long long hash_map_s2l_remove(void* map, const char* key);
static void hash_map_s2l_dispose(void* map);
static int strequ(const char*, const char*);
long long nanotime(); /* current time in nanoseconds */
static void* start;

static void* map() {
    if (start == NULL) {
        start = hash_map_s2l_create(100);
    }
    return start;
}
static void traceHumanReadable(const char* label, long long delta) {
    if (delta < 10LL * 1000) {
        trace("time: %s %lld nanoseconds", label, delta);
    } else if (delta < 10LL * 1000 * 1000) {
        trace("time: %s %lld microseconds", label, delta / 1000LL);
    } else if (delta < 10LL * 1000 * 1000 * 1000) {
        trace("time: %s %lld milliseconds", label, delta / (1000LL * 1000));
    } else {
        trace("time: %s %lld seconds", label, delta / (1000LL * 1000 * 1000));
    }
}

long long timestamp(const char* label) {
    /* returns 0 on first use and delta in nanoseconds on second call */
    void* m = map();
    long long t = nanotime();
    long long s = hash_map_s2l_remove(m, label);
    if (s == 0) {
        hash_map_s2l_put(m, label, t);
        return 0;
    } else {
        long long delta = t - s;
        traceHumanReadable(label, delta);
        return delta;
    }
}

static int strequ(const char* s1, const char* s2) {
    return s1 == NULL || s2 == NULL ? s1 == s2 : (s1 == s2 || strcmp(s1, s2) == 0);
}

long long nanotime() {
struct timespec tm = {0};
#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  tm.tv_sec = mts.tv_sec;
  tm.tv_nsec = mts.tv_nsec;
#else
  //on linux CLOCK_REALTIME == CLOCK_REALTIME_HR
  //CLOCK_REALTIME_HR is no longer available on linux
  //unsure about osx
  clock_gettime(CLOCK_REALTIME, &tm);
  return 1000000000LL * tm.tv_sec + tm.tv_nsec;
#endif

  return 1000000000LL * tm.tv_sec + tm.tv_nsec;
}

static const char* REMOVED = "this entry has been removed";

typedef struct {
    int capacity;
    char** keys;
    long long* values;
} hashmaps2l_t;

static void* hash_map_s2l_create(int capacity) {
    if (capacity < 4) {
        return NULL;
    } else {
        hashmaps2l_t* map = (hashmaps2l_t*)calloc(sizeof(hashmaps2l_t), sizeof(char*));
        if (map != NULL) {
            map->keys = (char**) calloc(capacity, sizeof(char*));
            map->values = (long long*) calloc(capacity, sizeof(long long));
            map->capacity = capacity;
            if (map->keys == NULL || map->values == NULL) {
                hash_map_s2l_dispose(map);
                map = NULL;
            }
        }
        return map;
    }
}

static int hash(const char* k, int capacity) { /* assumes k != NULL && k[0] != NULL */
    int h = *k++;
    while (*k) {
        h = h * 13 + *k++;
    }
    h = (h & 0x7FFFFFFF) % capacity;
    return h;
}

static int hash_map_s2l_put(void* m, const char* k, long long v) {
    hashmaps2l_t* map = (hashmaps2l_t*) m;
    if (v == 0 || k == NULL || k[0] == 0) {
        return 0;
    } else {
        int h = hash(k, map->capacity);
        int h0 = h;
        for (;;) {
            if (map->keys[h] == NULL || strequ(map->keys[h], k) || map->keys[h] == REMOVED) {
                break;
            }
            h = (h + 1) % map->capacity;
            if (h == h0) {
                return 0;
            }
        }
        if (map->keys[h] == NULL || map->keys[h] == REMOVED) {
            const char* s = strdup(k);
            if (s == NULL) {
                return 0;
            }
            map->keys[h] = (char*) s;
        }
        map->values[h] = v;
        return 1;
    }
}

static long long hash_map_s2l_remove(void* m, const char* k) {
    hashmaps2l_t* map = (hashmaps2l_t*) m;
    int h = hash(k, map->capacity);
    int h0 = h;
    for (;;) {
        if (map->keys[h] == NULL) {
            return 0;
        }
        if (strequ(map->keys[h], k)) {
            free(map->keys[h]);
            map->keys[h] = (char*) REMOVED;
            return map->values[h];
        }
        h = (h + 1) % map->capacity;
        if (h == h0) {
            return 0;
        }
    }
    return 0;
}

static void hash_map_s2l_dispose(void* m) {
    hashmaps2l_t* map = (hashmaps2l_t*) m;
    if (map != NULL) {
        if (map->keys != NULL) {
            int i = 0;
            while (i < map->capacity) {
                if (map->keys[i] != NULL && map->keys[i] != REMOVED) {
                    free(map->keys[i]);
                    map->keys[i] = NULL;
                }
                i++;
            }
            free(map->keys);
            map->keys = NULL;
        }
        if (map->values != NULL) {
            free(map->values);
            map->values = NULL;
        }
        free(map);
    }
}

//#endif /*__IHDR_RELEASE_BUILD__*/
