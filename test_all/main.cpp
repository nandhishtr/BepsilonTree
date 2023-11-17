#include "pch.h"

#include "gtest/gtest.h"
#include "BTree.h"
#include "DRAMLRUCache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"

#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"
#include "NVRAMCacheObjectKey.h"

#include "INode.h"

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	//::testing::GTEST_FLAG(filter) = "BPlusTree_DRAMLRU_Suite_1*";
	return RUN_ALL_TESTS();
}