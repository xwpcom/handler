#pragma once
#include "event.h"
#include "criticalsection.h"
#include "autolock.h"
#include "handlerinternaldata.h"
#include "looperimpl.h"
namespace Bear {
class LooperImpl;
class tagLooperInternalData
{
public:
	tagLooperInternalData(LooperImpl*);
	~tagLooperInternalData();
	void gc();

	LooperImpl			*mLooperImpl=nullptr;
	bool				mLooperRunning = false;//looper数量不会很多，所以没有必要用bit field
	bool				mAttachThread = false;//不创建_WorkThreadCB线程，而是attach到其他线程中
	bool				mBMQuit = false;//是否已收到BM_QUIT消息
	shared_ptr<Event>	mExitEvent;//退出线程时可触发此事件

	CriticalSection					mMessageLock;
	list<tagLoopMessageInternal> 	mMessageList;

#ifdef _CONFIG_CALC_EVER_MAX_SIZE
	//统计曾经出现过的最大消息数，可用来评估系统性能
	long mCurrentMessages;
	long mEverMaxMessages;
#endif

	vector<shared_ptr<Event>>	mExitEvents;
	weak_ptr<Looper>		mOwnerLooper;
	long						mExitCode = 0;
	long						mTimerCheckQuitLooper = 0;
	ULONGLONG					mTickStartQuit = 0;//用来计算此looper花多久才真正退出
	ULONGLONG					mTickDump= 0;

	//private:
	map<void*, shared_ptr<Handler>> mDestroyedHandlers;
	long	mTimerGC = 0;
	int		mSeqGC = 0;
};
}
