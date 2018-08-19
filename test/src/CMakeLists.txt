set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -O0 -std=c++11 -Wl,-rpath,../lib")
include_directories(${DIR_BASE_INCLUDE} ${DIR_MODEL_INCLUDE})
link_directories(${DIR_BASE_LIB} ${DIR_MODEL_LIB})

add_executable(client 
moduletestclient.cpp 
clientSession.cpp
main.cpp)


target_link_libraries(client base dl readline crypto.a rt pthread z)





#add_executable(test
#test.cpp)

#target_link_libraries(test base dl readline crypto.a rt pthread z ncurses)