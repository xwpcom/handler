﻿#pragma once
#include "handler.h"
#include "timernode.h"
#include "timerextrainfo.h"

namespace Bear {

struct tagCancelRunnable
{
	shared_ptr<Handler>				mHandler;
	shared_ptr<Runnable>			mRunnable;
};

using std::unordered_map;
class Looper;
struct tagHandlerData
{
	tagHandlerData(Handler* handler);
	~tagHandlerData();
	
	//什么时候需要调用本接口:
	//当一个object本身没有未决业务的概念，并且其child也可能没有，则需要调用本接口来保活
	//本质是至少采用一个shared_ptr来保活object
	//active object是指有未决业务的对象，并且当所有业务完结时自动销毁
	void SetActiveObject()
	{
		mPassive = false;
	}

	string LOOPER_SAFE GetName()const;
	void LOOPER_SAFE SetName(const string& name);

	Timer_t setTimer(uint32_t interval);
	void killTimer(Timer_t& timerId);

	virtual long SetTimerEx(UINT interval, shared_ptr<tagTimerExtraInfo> info = nullptr);
	long GetLiveChildrenCount();
	virtual int addChildHelper(weak_ptr<Handler> child);
	shared_ptr<Handler> FindObject_Impl(const string& url);
	shared_ptr<Handler> GetChild_Impl(LONG_PTR id);
	long GetChildCount()const;
	virtual void Dump(int level, bool includingChild = true);
	void OnPrepareDestructor();

	shared_ptr<Looper> mLooper;//在create成功之后有效
	Handler* mHandler = nullptr;
	string mObjectName;
	LONGLONG mTickDestroy = 0;//用来计算此handler从收到OnDestroy()到析构的时间

	shared_ptr<Handler> mSelfRef;	//确保在运行时this指针有效,用于looper和被动型object
	unordered_map<long*, weak_ptr<Handler>> mChildren;
	atomic<int> mChildCount;

	shared_ptr<Handler> mParent;
	//int64_t mId = 0;//可用来标记特定handler,或者存放context信息
	bool mPassive : 1;// = true;//被动型object,被动型是指可对外提供服务，但没有未决业务这个概念,需要采用mSelfRef保活
					  //被动型object需要调用AddChild挂在parent上面才能收到BM_DESTROY来销毁
					  //而主动型AddChild是可选的,业务完结时它能自动销毁
	bool mCreated : 1;// = false;//c++11不支持,c++20才支持
	bool mOnCreateCalled : 1;//=false
	bool mDestroy : 1;// = false;
	bool mDestroyMarkCalled : 1;
	bool mOnDestroyCalled : 1;//=false
	bool mIsLooper : 1;// = false;
	bool mMaybeLongBlock : 1;//=false
	//bool mTimerIdRewind : 1;//当mNextTimerId回绕后分配新mNextTimerId时要检测冲突
	Timer_t mNextTimerId = 0;

	shared_ptr<unordered_map<long, shared_ptr<tagTimerNode>>> mTimerMap;
	void RemoveAllTimer();

	shared_ptr<TimerManager> mTimerManager;//当设置timer时要引用looper中的TimerManager,是为了保证在析构时TimerManager是有效的
};

}
