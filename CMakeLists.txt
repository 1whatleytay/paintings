cmake_minimum_required(VERSION 3.17)
project(paintings)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external)

find_package(CURL REQUIRED)

add_executable(paintings
    include/paintings/analysis.h
    include/paintings/colors.h
    include/paintings/image.h
    include/paintings/options.h
    include/paintings/pool.h

    src/analysis.cpp
    src/colors.cpp
    src/image.cpp
    src/options.cpp
    src/pool.cpp

    src/main.cpp)

target_include_directories(paintings PRIVATE include)
target_link_libraries(paintings PRIVATE fmt stb nlohmann_json CLI11 CURL::libcurl csv2)
