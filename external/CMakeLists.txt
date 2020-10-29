set(FMT_TEST FALSE)
set(CSV2_BUILD_TESTS FALSE)
set(CLI11_BUILD_TESTS FALSE)

add_subdirectory(fmt)
add_subdirectory(csv2)
add_subdirectory(json)
add_subdirectory(CLI11)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb)