#include "pch.h"
#include <future>
#include "loger.h"

#define CATCH_CONFIG_MAIN
#include "catch2.hpp"

using namespace Bear;

class Break
{
public:
	~Break() {
		int x = 0;
	}
};

static Break gBreak;

TEST_CASE("Handler") {

	class MainLooper :public AppLooper
	{

	};

	make_shared<MainLooper>()->start();

	//REQUIRE(2 == 1);
}

TEST_CASE("Thread") {

	class Loop
	{
	public:
		void entry()
		{

		}

		void operator()()
		{
			printf("%s\r\n", __func__);

		}
	};

	auto f = []() {
		printf("%s\r\n", __func__);
		};
	
	Loop loop1;
	thread t1(loop1);
	thread t2(f);

	auto ret=async(launch::async, []() {
		printf("hello\r\n");
		  });
	Sleep(1000);
	t1.join();
	t2.join();

	
	
}

TEST_CASE("event") {
	class Loop
	{
	public:
		string mTag;
		void operator()()
		{
			logV(mTag)<<__func__;
			logW(mTag) << "hello";

		}
	};

	Loop t1;
	t1.mTag = "t1";

	Loop t2;
	t2.mTag = "t2";

	thread obj(t1);
	obj.join();

	thread obj2(t2);
	obj2.join();
}
