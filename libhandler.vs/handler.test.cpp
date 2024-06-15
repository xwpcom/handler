#include "pch.h"
#include <future>
#include "loger.h"
#include "looper.h"

#include <iostream>                // cout
#include <thread>                // thread
#include <mutex>                // mutex, unique_lock
#include <condition_variable>


#define CATCH_CONFIG_MAIN
#include "catch2.hpp"

using namespace Bear;
static const char* TAG = "test";

class Break
{
public:
	~Break() {
		int x = 0;
	}
};

static Break gBreak;

TEST_CASE("Handler") {

	class AppLooper :public MainLooper
	{
	};

	//make_shared<AppLooper>()->start();

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

TEST_CASE("condition_variable") {
	class Loop
	{
	public:
		string mTag;
		shared_ptr<mutex> mCS;
		shared_ptr<condition_variable> mEvent;
		void operator()()
		{
			if (mTag == "t1")
			{
				//logV(mTag) << __func__<<" lock#begin";
				//unique_lock<mutex> lock(*mCS);
				//logV(mTag) << __func__ << " lock#end";
				Sleep(1 * 1000);
				logV(mTag) << __func__ << " notify#begin";
				mEvent->notify_one();
				logV(mTag) << __func__ << " notify#end";
			}
			else if (mTag == "t2")
			{
				logV(mTag) << __func__ << " lock#begin";
				unique_lock<mutex> lock(*mCS);
				logV(mTag) << __func__ << " lock#end";

				logV(mTag) << "wait for event#begin";
				mEvent->wait(lock);
				logV(mTag) << "wait for event#end";
			}
		}
	};

	auto cs=make_shared<mutex>();
	auto evt=make_shared<condition_variable>();

	Loop t1;
	t1.mCS = cs;
	t1.mTag = "t1";
	t1.mEvent = evt;

	Loop t2;
	t2.mCS = cs;
	t2.mTag = "t2";
	t2.mEvent = evt;

	thread obj(t1);
	thread obj2(t2);

	obj.join();
	obj2.join();
}

TEST_CASE("mutex")
{
	static const char* TAG = "mutex";

	auto cs = make_shared<mutex>();
	auto evt = make_shared<condition_variable>();


	class Task
	{
		int mId = 0;
	public:
		shared_ptr<mutex> mCS;
		void operator()(int id) {
			{
				logV(TAG) << "id=" << id<<" try lock";
				unique_lock <mutex> lck(*mCS);
				logV(TAG) << "id=" << id << " locked";
			}

		}
	};

	Task items[1];
	thread threads[1];
	// spawn threads:
	for (int i = 0; i < 1; ++i)
	{
		auto t = items[i];
		t.mCS = cs;
		threads[i] = thread(t, i);
	}

	for (auto& th : threads)
		th.join();
}

TEST_CASE("Looper_mainLooper") {

	class AppLooper :public MainLooper
	{
		Timer_t mTimer_test = 0;

		void onCreate()
		{
			__super::onCreate();

			setTimer(mTimer_test, 1000);
		}

		void onTimer(Timer_t id)
		{
			if (id == mTimer_test)
			{
				static int idx = 0;
				++idx;
				logV(mTag) << "idx=" << idx;
				postQuitMessage(0);
				return;
			}

			__super::onTimer(id);
		}

	};

	for(int i=0;i<1;i++)
	{
		auto obj = make_shared<AppLooper>();
		obj->start();
	}
}

TEST_CASE("Looper_addChild") {

	class AppLooper :public MainLooper
	{
		Timer_t mTimer_test = 0;

		class DemoHandler2:public Handler
		{
			string mTag = "demo";
		public:
			DemoHandler2() {
				logV(mTag) << __func__ << " this=" << this;

			}
			~DemoHandler2() {
				logV(mTag) << __func__ << " this=" << this;

			}
		protected:
			Timer_t mTimer_delayExit = 0;
			void onCreate()
			{
				__super::onCreate();
				setTimer(mTimer_delayExit, 1000);
			}

			void onTimer(Timer_t id) {
				if (id == mTimer_delayExit)
				{
					currentLooper()->postQuitMessage();
					return;
				}

				__super::onTimer(id);
			}
		};

		void onCreate()
		{
			__super::onCreate();

			auto obj = make_shared<DemoHandler2>();
			addChild(obj);
		}

	};


	auto obj = make_shared<AppLooper>();
	obj->start();
}

TEST_CASE("createHandler") {

	class AppLooper :public MainLooper
	{
		Timer_t mTimer_test = 0;

		class DemoHandler :public Handler
		{
			string mTag = "demo";
		public:
			DemoHandler() {
				logV(mTag) << __func__ << " this=" << this;

			}
			~DemoHandler() {
				logV(mTag) << __func__ << " this=" << this;

			}
		protected:
			Timer_t mTimer_delayExit = 0;
			void onCreate()
			{
				__super::onCreate();
				setTimer(mTimer_delayExit, 1000);
			}

			void onTimer(Timer_t id) {
				if (id == mTimer_delayExit)
				{
					currentLooper()->postQuitMessage();
					return;
				}

				__super::onTimer(id);
			}
		};

		void onCreate()
		{
			__super::onCreate();

			auto obj = make_shared<DemoHandler>();
			addChild(obj);
			//obj->create();
		}

	};


	auto obj = make_shared<AppLooper>();
	obj->start();
}
