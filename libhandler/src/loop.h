#pragma once

#include "object.h"
#include "handler.h"
#include "timerextrainfo.h"
#include <memory>
namespace Bear {
using namespace std;
class Handler;
//tagLoopMessage供外界调用时使用
struct tagLoopMessage
{
	tagLoopMessage()
	{
		mMsg = 0;
		mWP = 0;
		mLP = 0;
		mHandler = nullptr;
	}

	shared_ptr<Handler> mHandler;
	int32_t	mMsg = 0;
	int64_t *mWP = nullptr;
	int64_t* mLP=nullptr;
};

//XiongWanPing 2016.01.08
//Loop抽象类提供外界调用的接口
class Loop :public Handler
{
	friend class Handler;
	friend class tagHandlerInternalData;
public:
	virtual ~Loop() {}

	virtual int Start() = 0;
	virtual int StartRun() = 0;

	virtual int PostQuitMessage(long exitCode = 0) = 0;
	virtual int GetQuitCode()const = 0;

	virtual int64_t sendMessage(shared_ptr<Handler> handler, int64_t msg, int64_t* wp = NULL, int64_t* lp = NULL) = 0;
	virtual int64_t postMessage(shared_ptr<Handler> handler, int64_t msg, int64_t* wp = NULL, int64_t* lp = NULL) = 0;
	virtual int64_t sendMessage(int64_t msg, int64_t* wp = NULL, int64_t* lp = NULL) = 0;
	virtual int64_t postMessage(int64_t msg, int64_t* wp = NULL, int64_t* lp = NULL) = 0;
	virtual bool IsRunning()const = 0;

	virtual void* GetLooperHandle()const = 0;

	virtual int wakeup() = 0;
	virtual int64_t tick()const = 0;
protected:
	virtual bool CanQuitLooperNow() = 0;
	virtual long SetTimer(Handler *handler, int64_t interval) = 0;
	virtual long SetTimer(shared_ptr<Handler> handler, int64_t interval) = 0;
	virtual void KillTimer(shared_ptr<Handler>handler, long& timerId) = 0;
	virtual void KillTimer(Handler *handler, long& timerId) = 0;
private:
	virtual long SetTimerEx(shared_ptr<Handler>handler, int64_t interval, shared_ptr<tagTimerExtraInfo> extraInfo) = 0;
	virtual long SetTimerEx(Handler *handler, int64_t interval, shared_ptr<tagTimerExtraInfo> extraInfo) = 0;
};
}
