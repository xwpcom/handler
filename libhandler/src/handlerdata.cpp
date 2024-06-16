#include "pch.h"
#include "handler.h"
#include "timernode.h"
#include "timermanager.h"
#include "handlerdata.h"
#include "looperdata.h"
#include "message.inl"
#include "system.h"
#include "loger.h"
#include "looper.h"

namespace Bear {
#define TAG "handlerData"
#define mTag "handlerData"

#ifdef _MSC_VER_DEBUG
//#define _TEST_TIMER_ID_REWIND	//测试timerId回绕
#endif

#ifdef _CONFIG_MONITOR_HANDLER
CriticalSection tagHandlerData::gCSBaseHandler;
std::unordered_map<long*, long*> tagHandlerData::gHandlers;
int tagHandlerData::gRationalHandlerUpperLimit = 0;// 1000;
#endif

static recursive_mutex	gInternalDataCS;
//目前仅在如下场景用到:AddChildHelper，用来避免多个looper同时对同一handler调用AddChild引起竞争
//Get/SetObjectName,保证跨looper访问mObjectName的安全
//AddChild/GetObjectName/SetObjectName都不会被非常频繁的调用，所以不太可能是瓶颈

tagHandlerData::tagHandlerData(Handler* handler)
{
	//要等到Create()时才和looper绑定

	mHandler = handler;
	mPassive = true;
	mCreated = false;
	mOnCreateCalled = false;
	mDestroy = false;
	mDestroyMarkCalled = false;
	mOnDestroyCalled = false;
	mIsLooper = false;
	mMaybeLongBlock = false;
	mTimerIdRewind = false;

	//LogV(TAG,"%s,this=%p", __func__, this);
#ifdef _TEST_TIMER_ID_REWIND
	mNextTimerId = -2;//for test
#endif

	//LogV(TAG,"sizeof(tagHandlerData)=%d", sizeof(tagHandlerData));//192
}

//corelooper框架保证tagHandlerData总是在handler的原生looper中析构(几乎是100%)

tagHandlerData::~tagHandlerData()
{
	//LogV(TAG,"%s,this=%p", __func__, this);

	if (!mIsLooper)
	{
		RemoveAllTimer();
	}

	if (mParent)
	{
		mParent->mInternalData->RemoveChildWeakRef(mHandler);

		mParent->mInternalData->mChildCount--;
	}
}

void tagHandlerData::RemoveAllTimer()
{
	auto& timerMap = mTimerMap;
	if (timerMap)
	{
		for (auto iter = timerMap->begin(); iter != timerMap->end();)
		{
			auto info = (*iter).second;
			int id = (int)info->mTimerId;
			//timerId<0为内部特殊timer,不要清除，目前timerId=-1表示looper定时检测是否能退出timer
			if (id >= 0)
			{
				info->mTimerManager->removeTimer(info.get());

				auto iterSave = iter++;
				timerMap->erase(iterSave);
			}
			else
			{
				++iter;
			}
			//info->mSlot->RemoveNode(&info->mNode);
		}

		if (timerMap->size() == 0)
		{
			timerMap = nullptr;
		}
	}
}

long tagHandlerData::GetLiveChildrenCount()
{
	//*
	if (mChildCount > 0)
	{
		return mChildCount;
	}
	//*/

	int count = 0;

	for (auto iter = mChildren.begin(); iter != mChildren.end();)
	{
		auto child = iter->second.lock();
		if (child)
		{
			++iter;
			++count;
		}
		else
		{
			auto iterSave = iter;
			++iter;
			mChildren.erase(iterSave);
		}
	}

	return count;
}

int tagHandlerData::addChildHelper(weak_ptr<Handler> wpChild)
{
	assert(mHandler->isSelfLooper());

	auto child = wpChild.lock();
	if (!child)
	{
		return -1;
	}

	bool callCreate = false;
	bool callDestroy = false;

	{
		if (child->mInternalData->mParent || (!child->isLooper() && child->isCreated()))
		{
			if (child->mInternalData->mParent)
			{
				logW(TAG)<<"child->mInternalData->mParent is NOT null";
			}

			if (child->isCreated())
			{
				logW(TAG)<<"child is already created";
			}

			logW(TAG)<<__func__ <<" fail";
			//assert(FALSE);
			return -1;
		}

		if (!child->isLooper())
		{
			//考虑跨looper创建普通handler
			auto parentLooper = dynamic_pointer_cast<Looper>(Looper::currentLooper()->shared_from_this());
			child->mThreadId = mHandler->mThreadId;
			child->mInternalData->mLooper = parentLooper;

			if (!parentLooper)
			{
				logW(TAG)<<"parentLooper is null";
			}
		}

		child->mInternalData->mParent = mHandler->shared_from_this();
		mChildCount++;
		
		mChildren[(long*)child.get()] = wpChild;

		if (child->mInternalData->mPassive)
		{
			child->mInternalData->mSelfRef = child->shared_from_this();//被动对象引用自己来保活,will be clear on BM_DESTROY
		}
		
		//适时触发初始化

		if (!child->mInternalData->mCreated && !child->isLooper())
		{
			callCreate = true;
		}

		//parent已销毁时，马上销毁child
		if (mDestroy)
		{
			callDestroy = true;
		}
	}

	auto ret = -1;
	//注意:callCreate和callDestroy同时为true时不要抵消，要按正常流程走完

	if (callCreate)
	{
		ret = (int)child->sendMessage(BM_CREATE);
	}
	
	if (callDestroy)
	{
		child->destroy();
	}

	return ret;
}

shared_ptr<Handler> tagHandlerData::FindObject_Impl(const string& url)
{
	return nullptr;
}

shared_ptr<Handler> tagHandlerData::GetChild_Impl(LONG_PTR id)
{
	return nullptr;
}

long tagHandlerData::GetChildCount()const
{
	return (long)mChildren.size();
}

void tagHandlerData::RemoveChildWeakRef(Handler *handler)
{
	if (mHandler->isSelfLooper())
	{
		RemoveChildWeakRef_Impl(handler);
	}
	else
	{
		mHandler->sendMessage(BM_REMOVE_CHILD_WEAKREF, (WPARAM)handler);
	}
}

void tagHandlerData::RemoveChildWeakRef_Impl(Handler *handler)
{
	assert(mHandler->isSelfLooper());

	if (!handler)
	{
		return;
	}

	auto iter = mChildren.find((long*)handler);
	if (iter != mChildren.end())
	{
		mChildren.erase(iter);

		//OnChildDetach();
	}
	else
	{
		assert(FALSE);
	}
}

//注意:setTimer/SetTimerEx/killTimer在Handler和Looper的实现是不同的，所以这里要区分

long tagHandlerData::SetTimerEx(UINT interval, shared_ptr<tagTimerExtraInfo> info)
{
	if (mHandler->isLooper())
	{
		auto obj = (Looper *)(mHandler);
		return obj->setTimerEx(mHandler, interval, info);
	}

	if (mLooper)
	{
		if (!mTimerManager)
		{
			auto obj = dynamic_cast<Looper*>(mLooper.get());
			mTimerManager = obj->GetTimerManager();
		}

		return mLooper->setTimerEx(mHandler, interval, info);
	}

	logW(mTag)<<__func__<<" fail due to mLooper is null";
	return 0;
}

Timer_t tagHandlerData::setTimer(uint32_t interval)
{
	if (mHandler->isLooper())
	{
		auto obj = (Looper *)(mHandler);
		return obj->setTimer(mHandler, interval);
	}

	if (mLooper)
	{
		if (!mTimerManager)
		{
			auto obj = dynamic_cast<Looper*>(mLooper.get());
			mTimerManager = obj->GetTimerManager();
		}

		return mLooper->setTimer(mHandler, interval);
	}

	logW(TAG)<<__func__<<"Fail due to mLooper is null";
	return 0;
}

void tagHandlerData::killTimer(Timer_t& timerId)
{
	if (mHandler->isLooper())
	{
		auto obj = (Looper *)(mHandler);
		obj->killTimer(mHandler, timerId);
		return;
	}

	if (!mLooper)
	{
		return;
	}

	mLooper->killTimer(mHandler, timerId);
}

string tagHandlerData::GetName()const
{
	string name;
	name.reserve(64);//should be enough for most case
	{
		AutoLock lock(gInternalDataCS);
		name = mObjectName;
	}

	return name;
}

void tagHandlerData::SetName(const string& name)
{
	auto str = name;
	
	//std move speed up
	AutoLock lock(gInternalDataCS);
	mObjectName= str;
}

void tagHandlerData::Dump(int level, bool includingChild)
{
}

//gc时如果没能成功析构Handler,可能多次调用本接口
//可用来检测不能及时析构的Handler
void tagHandlerData::OnPrepareDestructor()
{
	auto tickNow = tickCount();

	auto seconds = (tickNow - mTickDestroy) / 1000;
	if (seconds >= 10)
	{
		logW(TAG)<<GetName()<<" is still alive for "<<seconds<<" seconds after called Destroy()";
	}
}

}
