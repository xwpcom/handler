#include "pch.h"
//#undef GTEST_HAS_TR1_TUPLE
//#include <gtest/gtest.h>

#include "framework.h"


int APIENTRY wWinMain(_In_ HINSTANCE,
					  _In_opt_ HINSTANCE,
					  _In_ LPWSTR,
					  _In_ int)
{
	auto p = new int;
	return 0;
}

#define VLD_FORCE_ENABLE
#include <vld.h>
#include <iostream>

#include <memory>
using namespace std;


int TestVld()
{
	//VLDSetOptions(VLD_OPT_TRACE_INTERNAL_FRAMES | VLD_OPT_SKIP_CRTSTARTUP_LEAKS, 256, 64);

	void* m = malloc(50); // 8
	char* n = new char[60]; // 9

	class Demo
	{
	public:
		int x = 0;
		shared_ptr<Demo> mBuddy;
	};

	auto obj = make_shared<Demo>();
	obj->mBuddy = obj;


	// std libary dynamically initializes the objects "cout" and "cerr", which
	// produce false positive leaks in Release_StaticCrt because we doesn't have
	// debug CRT allocation header.
	std::cout << "Test: cout";
	//std::cerr << "Test: cerr";

	// At this point VLDGetLeaksCount() and VLDReportLeaks() should report 9 leaks
	// including a leak for ml which has not been freed yet. ml will be freed after
	// _tmain exits but before VLDReportLeaks() is called internally by VLD and
	// therefore correctly report 8 leaks.
	//int leaks = VLDGetLeaksCount();
	//VLDReportLeaks(); // at this point should report 9 leaks;
	return 0;
}

/*
General>Configuration type: Application (.exe)
linker>System>Sub system>console
注意:同时定义_tmain和wWinMain并且sub system选windows exe的话，在vs中不能正常启动,要去掉_tmain才能正常运行到wWinMain
*/
/*
int _tmain(int argc, _TCHAR* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	auto x = RUN_ALL_TESTS();

	//return TestVld();
	return 0;
}
//*/
//
/*
TEST(TestWinMain, RunExe)
{
	VLDMarkAllLeaksAsReported();

	//TestVld();
	auto p = new int;
}
*/

