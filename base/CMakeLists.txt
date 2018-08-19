cmake_minimum_required (VERSION 3.8)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -O3 -std=c++11")

if(MSVC)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0A00)
endif(MSVC)

include_directories(
    ${PROJECT_SOURCE_DIR}/base/base/3rd
    ${PROJECT_SOURCE_DIR}/base/base/3rd/lua
)

file(GLOB_RECURSE LUA_INC "${PROJECT_SOURCE_DIR}/base/base/3rd/lua/*.h")
file(GLOB_RECURSE LUA_C "${PROJECT_SOURCE_DIR}/base/base/3rd/lua/*.c")

file(GLOB_RECURSE HTTP_PARSER_INC "${PROJECT_SOURCE_DIR}/base/base/3rd/http_parser/*.h")
file(GLOB_RECURSE HTTP_PARSER_C "${PROJECT_SOURCE_DIR}/base/base/3rd/http_parser/*.c")

file(GLOB_RECURSE TINYXML_INC "${PROJECT_SOURCE_DIR}/base/base/3rd/tinyxml/*.h")
file(GLOB_RECURSE TINYXML_C "${PROJECT_SOURCE_DIR}/base/base/3rd/tinyxml/*.c")

file(GLOB_RECURSE INC_LIST "*.h")
file(GLOB_RECURSE CPP_LIST "*.cpp")

group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${LUA_INC})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${LUA_C})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${HTTP_PARSER_INC})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${HTTP_PARSER_C})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${TINYXML_INC})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${TINYXML_C})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${INC_LIST})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${CPP_LIST})


add_library(
    base 
    SHARED 
    ${LUA_INC}
    ${LUA_C}
    ${HTTP_PARSER_INC}
    ${HTTP_PARSER_C}
    ${TINYXML_INC}
    ${TINYXML_C}
    ${INC_LIST}
    ${CPP_LIST}
)

if(MSVC)
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/lib)
else()
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/lib)
endif(MSVC)