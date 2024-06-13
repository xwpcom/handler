#pragma once
#include <stdint.h>
#include <list>
//#include "handler.h"
#include "timernode.h"
#if 1
namespace Bear {

class Handler;
struct tagWheel;

//XiongWanPing 2016.07.04
class TimerManager
{
public:
	TimerManager();
	virtual ~TimerManager();
	int processTimer(uint32_t& cmsDelayNext,int64_t tick);
	void clearCacheTick();

	Timer_t setTimer(shared_ptr<Handler>handler, Timer_t timerId, uint32_t interval, shared_ptr<tagTimerExtraInfo> extraInfo = nullptr);
	void killTimer(shared_ptr<Handler>handler, Timer_t& timerId);

	Timer_t setTimer(Handler *handler, Timer_t timerId, uint32_t interval, shared_ptr<tagTimerExtraInfo> extraInfo = nullptr);
	void killTimer(Handler *handler, Timer_t& timerId);

	void removeTimer(tagTimerNode* node);
	void enableDebugInfo(bool enable)
	{
		mEnableDebugInfo = enable;
	}
private:
	long getMinIdleTime()const;
	void detectList();

	uint32_t cascade(uint32_t wheelIndex);
	void addTimer(uint32_t milseconds, tagTimerNode *node);
	void addToReady(tagTimerNode *node);
	void processTimeOut();

	static uint64_t getCurrentMillisec();

private:
	tagWheel *  mWheels[5];
	uint64_t	mStartTick;
	tagNodeLink	mReadyNodes;
	bool		mBusying;
	bool		mEnableDebugInfo = false;
	int64_t   mCacheTick=0;
};
}
#endif
