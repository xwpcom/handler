#pragma once
namespace Bear {
typedef int32_t Timer_t;

enum eCommand
{
	BM_NULL = 0,
	BM_CREATE,
	BM_DESTROY,			//调用Destroy()时触发
	BM_CHILD_DTOR,
	BM_QUIT,

	//corelooper internal use
	//BM_TIMER,//corelooper direct call OnTimer
	//BM_CLOSE = 2000000,
	//BM_QUIT = 2000000,
	//BM_AUTO_CREATE,
	BM_CREATE_EXIT_EVENT,

	BM_POST_DISPOSE,
	//BM_DUMP,
	//BM_ADD_CHILD,
	BM_FIND_OBJECT,
	BM_GET_CHILD,
	BM_GET_SHORTCUT,
	BM_REGISTER_SHORTCUT,
	BM_PRE_EXECUTE,
	BM_POST_EXECUTE,
	BM_HANDLER_DESTROY,

	BM_POST_RUNNABLE,
	BM_CANCEL_RUNNABLE,
	BM_SEND_RUNNABLE_FUNCTIONAL,//lambda

	BM_MESSAGE,
	BM_POST_MESSAGE,

	BM_PROFILER_START,
	BM_PROFILER_STOP,
	BM_PROFILER_DUMP,
};

class Object;
class Handler;
class Runnable;

struct tagAutoObject
{
public:
	virtual ~tagAutoObject() {}
	virtual void clear() {}
};

template<class T>
struct tagPostDisposeInfo :public tagAutoObject
{
	virtual ~tagPostDisposeInfo()
	{
		//DV("%s,this=%p",__func__,this);
	}

	void clear()
	{
		mSelfRef = nullptr;
	}

	shared_ptr<tagPostDisposeInfo> mSelfRef;
	shared_ptr<T> mSmartPtr;

};

struct tagTimerExtraInfo
{
	tagTimerExtraInfo() {}
	virtual ~tagTimerExtraInfo() {}

	shared_ptr<Runnable> mRunnable;
};

//仿android Runnable命名
class Runnable :public enable_shared_from_this<Runnable>
{
public:
	virtual ~Runnable() {}

	virtual void Run() = 0;
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
