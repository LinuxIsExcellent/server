cmake_minimum_required (VERSION 3.8)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wno-unused-local-typedefs -o3 -std=c++11")

if(MSVC)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0A00)
endif(MSVC)

include_directories(
    ${PROJECT_SOURCE_DIR}/base/base/3rd
    ${PROJECT_SOURCE_DIR}/base
    ${PROJECT_SOURCE_DIR}/model
)

file(GLOB_RECURSE INC_LIST "*.h")
file(GLOB_RECURSE CPP_LIST "*.cpp")

group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${INC_LIST})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${CPP_LIST})

add_library(
    engine 
    SHARED 
    ${INC_LIST}
	${CPP_LIST}
)

if(MSVC)
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/lib)
else()
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/lib)
endif(MSVC)