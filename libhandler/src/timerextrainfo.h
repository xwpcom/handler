#pragma once
namespace Bear {
typedef int32_t Timer_t;

class Object;
class Handler;
class Runnable;
struct tagTimerExtraInfo
{
	tagTimerExtraInfo() {}
	virtual ~tagTimerExtraInfo() {}

	shared_ptr<Runnable> mRunnable;
};

struct tagDelayedRunnable
{
	virtual ~tagDelayedRunnable() {}

	shared_ptr<tagDelayedRunnable> mSelfRef;
	shared_ptr<Handler>  mHandler;
	shared_ptr<Runnable> mRunnable;
	int32_t mDelayedMS = 0;//为0表示立即执行
};

struct tagSelfRef
{
	virtual ~tagSelfRef() {}

	shared_ptr<tagSelfRef> mSelfRef;
	shared_ptr<Object>		mObject;
};

}
