add_library(libbtree
            BeTree.hpp
            BeTreeInternalNode.hpp
            ErrorCodes.h
            BeTreeLeafNode.hpp
            BeTreeMessage.hpp
            BeTreeNode.hpp
            BeTreeLRUCache.hpp
            BeTreeFileStorage.hpp
            BeTreeNVMStorage.hpp
)


target_include_directories(libbtree PUBLIC
            "${PROJECT_SOURCE_DIR}/../libcache"
)

set_target_properties(libbtree PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(libbtree PUBLIC haldendb_compiler_flags)
