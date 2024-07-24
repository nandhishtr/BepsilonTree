add_library(libbtree
            BeTree.cpp
            BeTreeInternalNode.cpp
            ErrorCodes.h
            BeTreeLeafNode.cpp
            BeTreeMessage.cpp
            BeTreeNode.cpp
            BeTreeLRUCache.cpp
            BeTreeFileStorage.cpp
            BeTreeNVMStorage.cpp
)


target_include_directories(libbtree PUBLIC
            "${PROJECT_SOURCE_DIR}/../libcache"
)

set_target_properties(libbtree PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(libbtree PUBLIC haldendb_compiler_flags)