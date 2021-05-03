cmake_minimum_required(VERSION 3.17)
project(tbb_play)

set(CMAKE_CXX_STANDARD 17)

add_executable(tbb_play main.cpp)

target_link_libraries(tbb_play PRIVATE -ltbb)
target_link_directories(tbb_play PRIVATE /home/noriss/oneapi-tbb-2021.2.0/env/../lib/intel64/gcc4.8)

target_include_directories(tbb_play PRIVATE /home/noriss/oneapi-tbb-2021.2.0/include)