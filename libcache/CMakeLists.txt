add_library(libcache
            DRAMCacheObject.cpp
            DRAMCacheObjectKey.cpp
            DRAMLRUCache.cpp
            DRAMVolatileStorage.cpp
            NoCache.cpp
            NVRAMCacheObject.cpp
            NVRAMCacheObjectKey.cpp
            NVRAMDAXStorage.cpp
            NVRAMLRUCache.cpp
            NVRAMObjectStorage.cpp
            NVRAMVolatileStorage.cpp
            UnsortedMapUtil.cpp
            ICacheObject.h
)

target_link_libraries(libcache PUBLIC haldendb_compiler_flags)