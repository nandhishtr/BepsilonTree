cmake_minimum_required(VERSION 3.10)

project(sandbox VERSION 1.0)

find_package (glog 0.6.0 REQUIRED)

set(CMAKE_BUILD_TYPE Debug)

add_library(haldendb_compiler_flags INTERFACE)
target_compile_features(haldendb_compiler_flags  INTERFACE cxx_std_20)

#configure_file(TutorialConfig.h.in TutorialConfig.h)

#add_compile_definitions(__CONCURRENT__)
add_compile_definitions(__TREE_AWARE_CACHE__)

add_subdirectory(../libcache ${PROJECT_BINARY_DIR}/libcache)
add_subdirectory(../libbtree ${PROJECT_BINARY_DIR}/libbtree)


add_executable(sandbox sandbox.cpp)

target_compile_options(sandbox PRIVATE
    -ggdb3
    -O0
    -g3
    -Wall)

target_link_libraries(sandbox PUBLIC libcache haldendb_compiler_flags)
target_link_libraries(sandbox PUBLIC libbtree haldendb_compiler_flags)
target_link_libraries (sandbox PUBLIC glog::glog)

target_include_directories(sandbox PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${PROJECT_SOURCE_DIR}/../libcache"
                           "${PROJECT_SOURCE_DIR}/../libbtree"
                           )
