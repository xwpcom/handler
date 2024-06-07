#pragma once
#include <stdint.h>
#include <list>
#include "handler.h"
#include "timernode.h"
#if 0
namespace Bear {

struct tagWheel;

//XiongWanPing 2016.07.04
class TimerManager
{
public:
	TimerManager();
	virtual ~TimerManager();
	int ProcessTimer(uint32_t& cmsDelayNext,int64_t tick);
	void clearCacheTick();

	int setTimer(std::shared_ptr<Handler>handler, long timerId, int32_t interval, std::shared_ptr<tagTimerExtraInfo> extraInfo = nullptr);
	void killTimer(std::shared_ptr<Handler>handler, long& timerId);

	int setTimer(Handler *handler, long timerId, int32_t interval, std::shared_ptr<tagTimerExtraInfo> extraInfo = nullptr);
	void killTimer(Handler *handler, long& timerId);

	void RemoveTimer(tagTimerNode* node);
	void EnableDebugInfo(bool enable)
	{
		mEnableDebugInfo = enable;
	}
private:
	long GetMinIdleTime()const;
	void DetectList();

	uint32_t Cascade(uint32_t wheelIndex);
	void AddTimer(uint32_t milseconds, tagTimerNode *node);
	void AddToReady(tagTimerNode *node);
	void ProcessTimeOut();

	static uint64_t GetCurrentMillisec();

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
