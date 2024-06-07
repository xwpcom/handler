#include "pch.h"
#include <future>

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

	async(launch::async, []() {
		printf("hello\r\n");
		  });
	Sleep(1000);
	t1.join();
	t2.join();

	
	
}
