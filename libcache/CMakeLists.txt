add_library(libcache
            CacheErrorCodes.h
            FileStorage.hpp
            IFlushCallback.h
            LRUCache.hpp
            LRUCacheObject.hpp
            NoCache.hpp
            NoCacheObject.hpp
            ObjectFatUID.cpp
            ObjectFatUID.h
            UnsortedMapUtil.hpp
            VariadicNthType.h
            VolatileStorage.hpp
)

target_link_libraries(libcache PUBLIC haldendb_compiler_flags)