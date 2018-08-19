cmake_minimum_required(VERSION 2.8)
project(client_simulation)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -O0 -std=c++11 -Wl,-rpath,../lib")
get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)

include("${ROOT_DIR}/base/CMakeVar.txt")
include("${ROOT_DIR}/model/CMakeVar.txt")

include_directories("/opt/app/Theron-6.00.02/Include" ${DIR_BASE_INCLUDE} ${DIR_MODEL_INCLUDE})
link_directories("/opt/app/Theron-6.00.02/Lib/" ${DIR_BASE_LIB})


add_executable(client_simulation
client/memorychunk.cpp
client/memorypool.cpp
client/packet_base.cpp
client/packet.cpp
client/connector.cpp
client/platform.cpp
client/exception.cpp

dal/clientimpl.cpp
dal/frontserver.cpp
dal/character.cpp
dal/map.cpp
dal/misc.cpp
dal/alliance.cpp
dal/fuben.cpp

configure.cpp 
player.cpp 
main.cpp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CURRENT_LIST_DIR ON)
target_link_libraries(client_simulation base pthread dl z libtherond.a crypto.a)

install(TARGETS client_simulation RUNTIME DESTINATION bin)

