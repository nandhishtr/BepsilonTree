#include <iostream>

#include "BTree.h"
#include "NoCache.h"


int main() {
    BTree<int, int, NoCache<std::shared_ptr<INVRAMCacheObject>>>* m_ptrTree = new BTree<int, int, NoCache<std::shared_ptr<INVRAMCacheObject>>>(5);;

    return 0;
}
