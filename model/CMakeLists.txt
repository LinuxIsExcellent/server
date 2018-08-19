cmake_minimum_required (VERSION 3.8)

SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++11)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -O0 -std=c++11")

if(MSVC)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0A00)
endif(MSVC)

include_directories(
    ${PROJECT_SOURCE_DIR}/base
)

file(GLOB_RECURSE INC_LIST "*.h")
file(GLOB_RECURSE CPP_LIST "*.cpp")

group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${INC_LIST})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${CPP_LIST})

#${INC_LIST}
#${CPP_LIST}

add_library(
    model 
    SHARED 
    ${INC_LIST}
    ${CPP_LIST}
)

if(MSVC)
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/lib)
else()
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/lib)
endif(MSVC)