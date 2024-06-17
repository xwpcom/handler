﻿#include "pch.h"
#include "handlerdata.h"
#include "looperdata.h"
#include "looper.h"

using namespace std;

namespace Bear {

tagLooperData::tagLooperData(Looper* looper)
{
	mLooper = looper;
	mBMQuit = false;
	mLooperRunning = false;
	mAttachThread = false;
}

tagLooperData::~tagLooperData()
{

}
//清除过时的Handlers
//检测机制:
//用weak_ptr引用mDestroyedHandlers中缓存的shared_ptr
//清除mDestroyedHandlers中的shared_ptr
//然后检查weak_ptr.lock(),如果lock为null表示handler已析构，否则要重新添加到mDestroyedHandlers中
void tagLooperData::gc()
{
	mLooper->assertLooper();

	//ASSERT(IsMyselfThread());
	++mSeqGC;

	auto& items = mDestroyedHandlers;
	for (auto iter = items.begin(); iter != items.end();)
	{
		weak_ptr<Handler> handler = iter->second;
		{
			void *key = iter->first;
			{
				auto item = iter;
				++iter;

				if (item->second.use_count() != 1)//这里有其他looper有竞争也没有关系，下面的lock会最终判断
				{
					continue;
				}
			}

			//XiongWanPing 2018.07.27
			//这里有竞争关系，可能导致在其他thread中析构handler,描述如下:
			//当上面的use_count=1时运行到此，然后cpu切换到另一thread,在另一thread中weakptr lock得到shared_ptr
			//然后切换到本thread执行items[key] = nullptr;
			//然后再切换到另一thread析构shared_ptr才再切回到本thread
			//感觉这种时序只在理论上存在，实际上基本不会遇到
			//用构造时序来重现此场景

			auto handlerData = items[key]->mInternalData;//保证handler data在原生looper中析构
			items[key] = nullptr;//如果没有外部引用，此时handler会析构

			auto obj = handler.lock();
			if (obj)
			{
				//有外部引用
				items[key] = obj;
				//obj->OnPrepareDestructor();
			}
			else
			{
				//handler已析构
				items.erase(key);
				handlerData->mHandler = nullptr;
				//handlerData->RemoveAllTimer();//显式清除所有timer
			}
		}
	}

	if (items.size() == 0)
	{
		mLooper->killTimer(mTimerGC);
		mSeqGC = 0;
	}
}


}
