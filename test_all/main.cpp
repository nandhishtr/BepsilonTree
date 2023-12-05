#include "pch.h"

#include "gtest/gtest.h"

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::GTEST_FLAG(filter) = "Bulk_Insert_Search_Delete*";
	return RUN_ALL_TESTS();
}