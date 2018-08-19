#ifndef CLIENT_PLATFORM_H
#define CLIENT_PLATFORM_H

#include <stdint.h>

#if (defined _WIN32 || defined _WIN64)
#define HAVE_WINDOWS
#define WIN32_LEAN_AND_MEAN
#else

#endif

#define SAFE_DELETE(ptr) do { if (ptr != NULL) { delete ptr; ptr = NULL; } } while (0)

#ifdef ANDROID
    #include <android/log.h>
    #define HAVE_ANDROID
    #define DDD(...)  __android_log_print(ANDROID_LOG_DEBUG, "DDD",__VA_ARGS__)
#else
    #define DDD(...)
#endif

#ifdef __cplusplus
    extern "C" {
#endif

void DDDLog(const char* format);

#ifdef __cplusplus
    }
#endif
#endif
