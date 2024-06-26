﻿#include "pch.h"
#include <future>
#include "loger.h"
#include "looper.h"
#include "system.h"
#include <iostream>                // cout
#include <thread>                // thread
#include <mutex>                // mutex, unique_lock
#include <condition_variable>

// https://github.com/JohnnyHendriks/TestAdapter_Catch2/blob/main/Docs/Settings.md#dllrunner

#define CATCH_CONFIG_MAIN
#include "catch2.hpp"

using namespace Bear;
static const char* TAG = "test";

class CDebugBreak
{
public:
	~CDebugBreak() {
		int x = 0;
	}
};

static CDebugBreak gBreak;

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

TEST_CASE("Looper.mainLooper") {

	class AppLooper :public MainLooper
	{
		Timer_t mTimer_test = 0;

		void onCreate()
		{
			__super::onCreate();

			Looper obj;

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

	for(int i=0;i<2;i++)
	{
		auto obj = make_shared<AppLooper>();
		obj->start();
	}
}

TEST_CASE("Looper.addChild")
{

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

TEST_CASE("Looper.createHandler") 
{

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
				logV(mTag) << __func__ << "this=" << this;
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
			//obj->create(this);
			obj->create(this);
		}

	};


	auto obj = make_shared<AppLooper>();
	obj->start();
}

TEST_CASE("Looper.Handler") {

	class AppLooper :public MainLooper
	{
	};

}

TEST_CASE("Looper.childLooper") {

	class AppLooper :public MainLooper
	{
		string mTasg = "AppLooper";
	public:
		AppLooper()
		{
			logV(mTag) << __func__ << " this=" << this;
		}
		~AppLooper()
		{
			logV(mTag) << __func__ << " this=" << this;
		}

	protected:
		void onCreate()
		{
			__super::onCreate();


			class DemoLooper :public Looper
			{
			public:
				DemoLooper() {
					logV(mTag) << __func__ << " this=" << this;
				}
				~DemoLooper() {
					logV(mTag) << __func__ << " this=" << this;
				}

			protected:
				void onCreate()
				{
					__super::onCreate();

					setTimer(mTimer_delayExit, 100);
				}

				Timer_t mTimer_delayExit = 0;
				void onTimer(Timer_t id)
				{
					if (id == mTimer_delayExit)
					{
						logV(mTag) << "postQuit";
						getMainLooper()->sendMessage(BM_NULL);
						getMainLooper()->postQuitMessage();
						return;
					}

					__super::onTimer(id);
				}

			};

			auto obj = make_shared<DemoLooper>();
			addChild(obj);
			obj->start();
		}
	};

	for (int i = 0; i < 10; i++)
	{
		{
			auto obj = make_shared<AppLooper>();
			obj->start();
		}

		//Sleep(1000);
	}
}

TEST_CASE("Looper.SendMessageSpeed_windows")
{
	auto hwnd = GetDesktopWindow();
	auto count = 1000 * 100;
	auto tick = tickCount();
	for (int i = 0; i < count; i++)
	{
		::SendMessage(hwnd, WM_NULL, 0, 0);
	}

	tick = tickCount() - tick;
	logI("windows")<< "count="<<count<<",tick="<<tick<<",perSecond="<< count * 1000.0 / tick;
	//i7-4790K上面 count=1000000,tick=15688,perSecond=63743
	//i7-12700上面 count=1000000,tick=6922,perSecond=144467
}

TEST_CASE("Looper.sendMessage") 
{

	class AppLooper :public MainLooper
	{
		Timer_t mTimer_test = 0;

		void onCreate()
		{
			__super::onCreate();

			class DemoLooper :public Looper
			{
			public:
				DemoLooper() {
					//logV(mTag) << __func__ << " this=" << this;
				}
				~DemoLooper() {
					//logV(mTag) << __func__ << " this=" << this;
				}

			protected:
				void onCreate()
				{
					__super::onCreate();

					/*
					{
						HANDLE hThread = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, GetCurrentThreadId());
						DWORD_PTR value = SetThreadAffinityMask(hThread, 0x0001);
					}
					//*/

				}
			};

			auto obj = make_shared<DemoLooper>();
			addChild(obj);
			obj->start();

			/*
			{
				HANDLE hThread = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, GetCurrentThreadId());
				DWORD_PTR value = SetThreadAffinityMask(hThread, 0x0001);
			}
			//*/

			{
				auto tick = tickCount();
				int count = 1000 * 100;
				for (int i = 0; i < count; i++)
				{
					obj->sendMessage(BM_NULL);
				}

				tick = tickCount() - tick;
				logI(mTag) << "crossLooper,count=" << count << ",tick=" << tick << ",perSecond=" << count * 1000.0 / tick;
			}
			{
				auto tick = tickCount();
				int count = 1000 * 1000;
				for (int i = 0; i < count; i++)
				{
					sendMessage(BM_NULL);
				}

				tick = tickCount() - tick;
				logI(mTag) << "localLooper,count=" << count << ",tick=" << tick << ",perSecond=" << (int64_t)(count * 1000.0 / tick);
			}

			//不绑定cpu时
			//count=1000000,tick=10062,perSecond=99383.8
			
			// 绑定到同一cpu时
			//i7 :count=1000000,tick=3250,perSecond=307692
			//i12:count=1000000,tick=4375,perSecond=228571
			setTimer(mTimer_test, 1);

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

	for (int i = 0; i < 1; i++)
	{
		auto obj = make_shared<AppLooper>();
		obj->start();
	}
}
