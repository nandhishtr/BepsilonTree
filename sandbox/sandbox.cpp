#include <iostream>

#include "BTree.h"
#include "NoCache.h"


int main() {
    BTree<int, int, NoCache<std::shared_ptr<INVRAMCacheObject>>>* m_ptrTree = new BTree<int, int, NoCache<std::shared_ptr<INVRAMCacheObject>>>(5);

    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        m_ptrTree->insert(nCntr, nCntr);
    }

    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        std::cout << "K: " << nCntr << ", V: " << nValue << std::endl;
        //ASSERT_EQ(nValue, nCntr);
    }

    return 0;
}
