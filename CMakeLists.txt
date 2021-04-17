cmake_minimum_required(VERSION 3.0.2)
project (repair-inplace)


if(NOT CMAKE_BUILD_TYPE)
    #set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

set(CXX_STANDARD gnu++17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -march=native -std=${CXX_STANDARD} ")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -ggdb -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -mtune=native")

add_subdirectory(external/bit_span)
include_directories(external/bit_span/include)

FIND_LIBRARY(GLOG_LIBRARY glog)
FIND_PATH(GLOG_INCLUDE_DIR "glog/logging.h")
add_executable  (repair-inplace repair.cpp)
target_link_libraries(repair-inplace  ${GLOG_LIBRARY} -lstdc++fs)

MESSAGE( STATUS "Built Type: " ${CMAKE_BUILD_TYPE} )
MESSAGE( STATUS "CXX Flags: " ${CMAKE_CXX_FLAGS} )
