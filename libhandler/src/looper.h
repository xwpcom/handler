#pragma once
#include "handler.h"
#include <thread>
#include <mutex>
namespace Bear {

enum eCommand
{
	BM_NULL = 0,
	BM_CREATE,
	BM_DESTROY,			//调用Destroy()时触发
	BM_CHILD_DTOR,
};

typedef lock_guard<recursive_mutex> AutoLock;

class tagLooperMessage
{
public:
	shared_ptr<Handler> handler;
	uint32_t cmd =0;
	int64_t  wp  = 0;
	int64_t  lp  = 0;


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
	friend class SmartTlsLooper;
	friend class SmartTlsLooperManager;
public:
	Looper();
	
	//只有AppLooper及其子类MainLooper为阻塞，其他looper的都是非阻塞
	void start();
	Looper& create(function<void()>) { return *this; }
protected:
	void onCreate();
	int run();
	int getMessage(tagLooperMessage& msg);
	int64_t dispatchMessage(tagLooperMessage& msg);
	int64_t onThreadMessage(tagLooperMessage& msg);
	int wakeup();
	bool canQuit();
	void assertLooper();
	void singleStep();

	int64_t postMessage(uint32_t cmd, int64_t wp = 0, int64_t lp=0);
	int64_t sendMessage(uint32_t cmd, int64_t wp = 0, int64_t lp = 0);
	int64_t sendMessage(shared_ptr<Handler> handler, int32_t msg, int64_t wp, int64_t lp);
	int64_t postMessage(shared_ptr<Handler>handler, int32_t msg, int64_t wp, int64_t lp);
	void sendMessageHelper(tagLooperMessage& msg, Looper& looper);
	bool postQueuedCompletionStatus(HANDLE handle=INVALID_HANDLE_VALUE, int32_t bytes=0, void* key=nullptr, LPOVERLAPPED overlapped=nullptr);
	void _StackLooperSendMessage(tagLooperMessage& loopMsg);

	static Looper* currentLooper();
	static void setCurrentLooper(Looper* looper);

private:
	int64_t mLooperTick = 0;
	unique_ptr<tagLooperData> mLooperData;

	HANDLE mLooperHandle = INVALID_HANDLE_VALUE;
};

class MainLooper :public Looper {
public:
	MainLooper()
	{
	}

};

}