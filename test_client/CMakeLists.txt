cmake_minimum_required (VERSION 3.8)

SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++11)
set(CMAKE_BUILD_TYPE Debug)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -O0 -std=c++11")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -O0 -std=c++11 -Wl,-rpath,../lib")
endif()

link_directories(${PROJECT_SOURCE_DIR}/bin/lib)

if(MSVC)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0A00)
endif(MSVC)

include_directories(
    ${PROJECT_SOURCE_DIR}/base
    ${PROJECT_SOURCE_DIR}/model
    ${PROJECT_SOURCE_DIR}/engine
)

link_directories(${PROJECT_SOURCE_DIR}/bin "/lib/x86_64-linux-gnu")

file(GLOB_RECURSE INC_LIST "*.h")
file(GLOB_RECURSE CPP_LIST "*.cpp")

group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${INC_LIST})
group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} ${CPP_LIST})

add_executable(
    test_client
    ${INC_LIST}
    ${CPP_LIST}
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(test_client base model engine dl readline crypto.a pthread z)
else()
    target_link_libraries(test_client base model engine dl readline crypto.a rt pthread z)
endif()

if(MSVC)
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
else()
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
endif(MSVC)