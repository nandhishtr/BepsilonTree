#include "pch.h"

#include "gtest/gtest.h"

//#define __CONCURRENT__
//#define __POSITION_AWARE_ITEMS__

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	//::testing::GTEST_FLAG(filter) = "BPlusStore_LRUCache_VolatileStorage_Suite*";
	return RUN_ALL_TESTS();
}