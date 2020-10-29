cmake_minimum_required(VERSION 3.17)
project(paintings)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external)

add_executable(analyze-true analyze-true.cpp)
target_link_libraries(analyze-true PRIVATE fmt stb)

add_executable(analyze-hue analyze-hue.cpp)
target_link_libraries(analyze-hue PRIVATE fmt stb nlohmann_json CLI11)

add_executable(create-sample create-sample.cpp)
target_link_libraries(create-sample PRIVATE fmt csv2 CLI11)
