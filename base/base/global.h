#ifndef BASE_GLOBAL_H
#define BASE_GLOBAL_H

/*
#ifndef __GNUC__
#error "only support gcc"
#endif

#if (__GNUC__ >=4) && (__GNUC_MINOR__ >= 6)
#else
#error "not supported gcc version"
#endif
*/

#include <cstdint>
#define DISABLE_COPY(Class) \
    Class(const Class&) = delete; \
    Class & operator=(const Class&) = delete;

#include <cstddef>
#include <iostream>

#define SAFE_DELETE(ptr) do { if (ptr != nullptr) { delete ptr; ptr = nullptr; } } while(0)

#define WARN_IF_UNUSED __attribute__ ((warn_unused_result))

#define FIRE_DEPRECATED __attribute__ ((deprecated))

#define FOREACH_DELETE_MAP(map) \
        for (auto it = map.begin(); it != map.end(); ++it) {\
            if (it->second) {\
                delete it->second;\
            }\
        }\
        map.clear()

#define FOREACH_DELETE_LIST(list) \
        for (auto it = list.begin(); it != list.end(); ++it) {\
            if (*it) {\
                delete *it;\
            }\
        }\
        list.clear()

#define FOREACH_DELETE_VECTOR(vector) \
        for (auto it = vector.begin(); it != vector.end(); ++it) {\
            if (*it) {\
                delete *it;\
            }\
        }\
        vector.clear()

#endif

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
