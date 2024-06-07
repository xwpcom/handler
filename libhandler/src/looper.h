#pragma once
#include "handler.h"
#include <thread>
#include <mutex>
namespace Bear {

typedef lock_guard<recursive_mutex> AutoLock;

class tagLooperMessage
{
	bool*		done = nullptr;
	bool		waitAck = false;
	int64_t*	ack = nullptr;
	Looper*		sendLooper = nullptr;
};

struct tagLooperData
{
	tagLooperData(Looper*) {}
	~tagLooperData() {}
	void gc() {}

	Looper* mLooper= nullptr;
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
	long	mTimerGC = 0;
	int		mSeqGC = 0;
};

class Looper :public Handler {
public:
	Looper();
	
	//只有AppLooper及其子类MainLooper为阻塞，其他looper的都是非阻塞
	void start();
protected:
	void onCreate();
	int run();
	int getMessage(tagLooperMessage& msg);
	bool canQuit();
	void assertLooper();

private:
	int64_t mLooperTick = 0;
	unique_ptr<tagLooperData> mLooperData;
};

class AppLooper :public Looper {
public:
	AppLooper()
	{
	}

};

}