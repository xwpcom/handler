#pragma once
#include "handler.h"
#include "looperdata.h"
namespace Bear {
class TimerManager;

class Looper :public Handler {
	friend class Handler;
	friend struct tagHandlerData;
	friend class SmartTlsLooper;
	friend class SmartTlsLooperManager;
public:
	Looper();
	~Looper();
	
	static Looper* getMainLooper();
	static int setMainLooper(Looper*);
	static bool isMainLooper(Looper* looper);

	//只有AppLooper及其子类MainLooper为阻塞，其他looper的都是非阻塞
	virtual void start();
	Looper& create(function<void()>) { return *this; }
	bool isRunning()const;
protected:
	virtual int64_t onMessage(uint32_t msg, int64_t wp = 0, int64_t lp = 0);
	void onBMQuit();
	int startHelper(bool newThread);
	static void* _WorkThreadCB(void* p);
	void* _WorkThread();

	void onCreate();
	int run();
	int getMessage(tagLooperMessage& msg);
	int64_t dispatchMessage(tagLooperMessage& msg);
	int64_t onThreadMessage(tagLooperMessage& msg);
	int wakeup();
	bool canQuit();
	void assertLooper();
	void singleStep();
	int postQuitMessage(long exitCode);
	
	virtual Timer_t setTimer(Timer_t& timerId, uint32_t interval);
	virtual void killTimer(Timer_t& timerId);
	virtual Timer_t setTimer(shared_ptr<Handler>handler, uint32_t interval);
	virtual void killTimer(shared_ptr<Handler>handler, Timer_t& timerId);

	virtual Timer_t setTimer(Handler* handler, uint32_t interval);//为了支持在Handler子类的构造函数中能调用SetTimer,构造函数中不能使用shared_from_this()
	virtual void killTimer(Handler* handler, Timer_t& timerId);//为了支持在Handler子类的构造函数中能调用KillTimer
	virtual Timer_t setTimerEx(uint32_t interval, shared_ptr<tagTimerExtraInfo> info = nullptr);
	virtual Timer_t setTimerEx(shared_ptr<Handler>handler, uint32_t interval, shared_ptr<tagTimerExtraInfo> info = nullptr);
	virtual Timer_t setTimerEx(Handler* handler, uint32_t interval, shared_ptr<tagTimerExtraInfo> info = nullptr);

	int64_t postMessage(uint32_t cmd, int64_t wp = 0, int64_t lp=0);
	int64_t sendMessage(uint32_t cmd, int64_t wp = 0, int64_t lp = 0);
	int64_t sendMessage(shared_ptr<Handler> handler, int32_t msg, int64_t wp, int64_t lp);
	int64_t postMessage(shared_ptr<Handler>handler, int32_t msg, int64_t wp, int64_t lp);
	void sendMessageHelper(tagLooperMessage& msg, Looper& looper);
	bool postQueuedCompletionStatus(HANDLE handle=INVALID_HANDLE_VALUE, int32_t bytes=0, void* key=nullptr, LPOVERLAPPED overlapped=nullptr);
	void _StackLooperSendMessage(tagLooperMessage& loopMsg);

	static Looper* currentLooper();
	static void setCurrentLooper(Looper* looper);

	shared_ptr<TimerManager> GetTimerManager();
	int processTimer(uint32_t& cmsDelayNext, int64_t ioIdleTick);

	string mThreadName = "looper";
private:
	int64_t mLooperTick = 0;
	unique_ptr<tagLooperData> mLooperData;

	HANDLE mLooperHandle = INVALID_HANDLE_VALUE;
	shared_ptr<TimerManager> mTimerManager;
	int64_t mLastIoTick = 0;

};

class MainLooper :public Looper {
public:
	MainLooper()
	{
		auto name = "MainLooper";
		setObjectName(name);
		mThreadName = name;
		setMainLooper(this);
	}
	
	virtual void start()
	{
		if (currentLooper())
		{
			assert(false);
			return;
		}

		bool newThread = false;
		startHelper(newThread);
	}

};

}