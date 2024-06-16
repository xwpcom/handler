#pragma once
#include <thread>
#include <mutex>
#include "event.h"
#include "handlerdata.h"
namespace Bear {
class Looper;

typedef lock_guard<recursive_mutex> AutoLock;

class tagLooperMessage
{
public:
	shared_ptr<Handler> handler;
	uint32_t cmd = 0;
	int64_t  wp = 0;
	int64_t  lp = 0;


	bool* done = nullptr;
	bool		waitAck = false;
	int64_t* ack = nullptr;
	Looper* sendLooper = nullptr;
};

struct tagLooperData
{
	tagLooperData(Looper*);
	~tagLooperData();
	void gc();

	Looper* mLooper = nullptr;
	bool				mStarted = false;
	bool				mLooperRunning = false;//looper数量不会很多，所以没有必要用bit field
	bool				mAttachThread = false;//不创建_WorkThreadCB线程，而是attach到其他线程中
	bool				mBMQuit = false;//是否已收到BM_QUIT消息
	//shared_ptr<Event>	mExitEvent;//退出线程时可触发此事件

	recursive_mutex	mMutex;
	list<tagLooperMessage> 	mMessageList;

	#ifdef _CONFIG_CALC_EVER_MAX_SIZE
	//统计曾经出现过的最大消息数，可用来评估系统性能
	long mCurrentMessages;
	long mEverMaxMessages;
	#endif

	//vector<shared_ptr<Event>>	mExitEvents;
	weak_ptr<Looper>			mOwnerLooper;
	long						mExitCode = 0;
	long						mTimerCheckQuitLooper = 0;
	ULONGLONG					mTickStartQuit = 0;//用来计算此looper花多久才真正退出
	ULONGLONG					mTickDump = 0;

	//private:
	map<void*, shared_ptr<Handler>> mDestroyedHandlers;
	Timer_t	mTimerGC = 0;
	int		mSeqGC = 0;

};
}
