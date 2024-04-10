cmake_minimum_required(VERSION 3.10)

project(test_all VERSION 1.0)

find_package (glog 0.7.0 REQUIRED)

add_library(haldendb_compiler_flags INTERFACE)
target_compile_features(haldendb_compiler_flags  INTERFACE cxx_std_20)

#configure_file(TutorialConfig.h.in TutorialConfig.h)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(../libcache ${PROJECT_BINARY_DIR}/libcache)
add_subdirectory(../libbtree ${PROJECT_BINARY_DIR}/libbtree)

set(CMAKE_CXX_COMPILER g++-11)

add_executable(test_all 
               BPlusStore_LRUCache_FileStorage_Suite_1.cpp 
               BPlusStore_LRUCache_FileStorage_Suite_2.cpp 
               BPlusStore_LRUCache_FileStorage_Suite_3.cpp
               BPlusStore_LRUCache_VolatileStorage_Suite_1.cpp
               BPlusStore_LRUCache_VolatileStorage_Suite_2.cpp
               BPlusStore_LRUCache_VolatileStorage_Suite_3.cpp
               BPlusStore_NoCache_Suite_1.cpp 
               BPlusStore_NoCache_Suite_2.cpp 
               BPlusStore_NoCache_Suite_3.cpp 
               main.cpp 
)

set_target_properties(test_all PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(test_all PUBLIC libcache haldendb_compiler_flags)
target_link_libraries(test_all PUBLIC libbtree haldendb_compiler_flags)
target_link_libraries(test_all PUBLIC glog::glog)

target_include_directories(test_all PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${PROJECT_SOURCE_DIR}/../libcache"
                           "${PROJECT_SOURCE_DIR}/../libbtree"
                           )

enable_testing()

target_link_libraries(test_all PUBLIC  GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_all)
