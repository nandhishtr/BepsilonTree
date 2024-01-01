add_library(libbtree
            BTree.cpp
            InternalNode.cpp
            LeafNode.cpp
)


target_include_directories(libbtree PUBLIC
            "${PROJECT_SOURCE_DIR}/../libcache"
)

target_link_libraries(libbtree PUBLIC haldendb_compiler_flags)