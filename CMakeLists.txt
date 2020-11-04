cmake_minimum_required(VERSION 3.17)
project(paintings)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external)

find_package(CURL REQUIRED)

# This is super disorganized...
add_library(paintings-tools
    include/paintings/analysis.h
    include/paintings/colors.h
    include/paintings/image.h
    include/paintings/options.h
    include/paintings/pool.h

    src/analysis.cpp
    src/colors.cpp
    src/image.cpp
    src/options.cpp
    src/pool.cpp)
target_include_directories(paintings-tools PUBLIC include)
target_link_libraries(paintings-tools PUBLIC fmt stb CLI11)

add_executable(paintings src/main.cpp)
target_link_libraries(paintings PRIVATE nlohmann_json CURL::libcurl csv2 paintings-tools)

add_executable(paintings-convert convert.cpp)
target_link_libraries(paintings-convert paintings-tools)
