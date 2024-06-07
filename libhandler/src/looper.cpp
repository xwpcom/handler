#include "pch.h"
#include "looper.h"
#include "system.h"

namespace Bear {

Looper::Looper()
{
}

void Looper::assertLooper()
{

}

void Looper::onCreate()
{
}

void Looper::start()
{
	run();
}

int Looper::run()
{
	return 0;
}

/*
返回0表示获取到消息
否则返回-1
*/
int Looper::getMessage(tagLooperMessage& msg)
{
	mLooperTick = tickCount();

	do
	{
		{
			AutoLock lock(mLooperData->mMutex);
			auto& msgList = mLooperData->mMessageList;
			if (!msgList.empty())
			{
				msg = msgList.front();
				msgList.pop_front();
				//mLastIoTick = mLooperTick;
				return 0;
			}
		}

		if (mLooperData->mBMQuit && canQuit())
		{
			return -1;
		}
	} while (0);
	return -1;
}

bool Looper::canQuit()
{
	assert(mLooperData->mBMQuit);
	#if 0
	auto& events = mLooperData->mExitEvents;
	while (events.size())
	{
		#ifdef _MSC_VER
		BOOL waitAll = TRUE;
		HANDLE arr[MAXIMUM_WAIT_OBJECTS];
		if (events.size() <= COUNT_OF(arr))
		{
			for (UINT i = 0; i < events.size(); i++)
			{
				arr[i] = events[i]->operator HANDLE();
			}

			int ret = WaitForMultipleObjects((DWORD)events.size(), arr, waitAll, 0);
			if (ret == WAIT_OBJECT_0)
			{
				events.clear();
				break;
			}
			else
			{
				return false;
			}
		}
		else
		{
			for (UINT i = 0; i < events.size(); i++)
			{
				bool signal = events[i]->Wait(0);
				//LogV(TAG,"events[%d],signal=%d",i,signal);
				if (signal)
				{
				}
				else
				{
					return false;
				}
			}

			events.clear();
		}
		#else
		for (UINT i = 0; i < events.size(); i++)
		{
			bool signal = events[i]->Wait(0);
			//LogV(TAG,"events[%d],signal=%d",i,signal);
			if (signal)
			{
			}
			else
			{
				return false;
			}
		}

		events.clear();
		#endif
	}

	{
		ULONGLONG tick = mLooperData->mTickStartQuit;
		ULONGLONG tickNow = ShellTool::GetTickCount64();
		int ms = 3000;
		if (tickNow >= tick + ms && tickNow >= mLooperData->mTickDump + ms)
		{
			mLooperData->mTickDump = tickNow;

			LogV(TAG, "after emit quit %d ms,following handlers still alive:", ms);
			mInternalData->Dump(0);//dump which object are still live
		}
	}
	//有未决事务时，应该等到所有事务完结后才能安全退出looper
	auto nc = mInternalData->GetLiveChildrenCount();
	return nc == 0;

	#endif

	return true;
}


}