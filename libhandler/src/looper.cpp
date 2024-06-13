#include "pch.h"
#include "looper.h"
#include "system.h"
#include "loger.h"
#include "looperdata.h"
#include "handlerdata.h"
#include "timermanager.h"

#ifdef _MSC_VER
#pragma comment(lib,"ws2_32.lib")
#endif

namespace Bear {

static __declspec(thread) Looper * gBaseLooper = NULL;

Looper::Looper()
{
	mTag = "looper";
	mLooperData = make_unique<tagLooperData>(this);
	mLooperData->mLooper = this;
}

void Looper::setCurrentLooper(Looper* looper)
{
	if (looper)
	{
		assert(!gBaseLooper);
	}

	gBaseLooper = looper;
}

Looper* Looper::currentLooper()
{
	if (!gBaseLooper)
	{
		//LogV(TAG,"gBaseLooper is null");
	}

	return gBaseLooper;
}


/*
返回0表示获取到消息
否则返回-1
*/
int Looper::getMessage(tagLooperMessage& msg)
{
	mLooperTick = tickCount();

	do
	{
		{
			AutoLock lock(mLooperData->mMutex);
			auto& msgList = mLooperData->mMessageList;
			if (!msgList.empty())
			{
				msg = msgList.front();
				msgList.pop_front();
				//mLastIoTick = mLooperTick;
				return 0;
			}
		}

		if (mLooperData->mBMQuit && canQuit())
		{
			return -1;
		}

		uint32_t msDelayNext = INFINITE;
		processTimer(msDelayNext, mLooperTick - mLastIoTick);
		msDelayNext = max(1, msDelayNext);

		{
			//assert(msDelayNext);//如果为0，会形成busy loop,占用大量cpu

			BOOL fAlertable = FALSE;
			ULONG count = 0;
			OVERLAPPED_ENTRY items[100];
			BOOL ok = GetQueuedCompletionStatusEx(mLooperHandle
												  , items
												  , _countof(items)
												  , &count
												  , msDelayNext
												  , fAlertable
			);

			if (!ok)
			{
				return -1;
			}

			if (count > 0 && mTimerManager)
			{
				mLastIoTick = mLooperTick;
			}

			#if defined _CONFIG_PROFILER
			if (mEnableProfiler)
			{
				mProfiler->ioCount += count;
			}
			#endif
#if 0
			for (ULONG i = 0; i < count; i++)
			{
				auto& item = items[i];
				auto ptr = item.lpCompletionKey;
				auto ov = item.lpOverlapped;
				auto bytes = item.dwNumberOfBytesTransferred;

				if (ptr > 0xFFFF)
				{
					assert(ov);

					IocpObject* obj = (IocpObject*)ptr;
					IoContext* context = CONTAINING_RECORD(ov, IoContext, mOV);

					#if defined _CONFIG_PROFILER

					//避免在下面的DispatchIoContext中修改mEnableProfiler
					const auto enableProfiler = mEnableProfiler;

					ULONGLONG tick = 0;
					if (enableProfiler)
					{
						tick = ShellTool::GetTickCount64();
					}
					#endif

					obj->DispatchIoContext(context, bytes);
					//此时obj可能已失效

					#if defined _CONFIG_PROFILER
					if (enableProfiler)
					{
						tick = ShellTool::GetTickCount64() - tick;

						if (mEnableProfiler)
						{
							if (tick > mProfiler->ioContextMaxTick)
							{
								mProfiler->ioContextMaxTick = tick;
							}

						}
					}
					#endif
				}
			}
#endif
			if (mLooperData->mAttachThread)
			{
				// stack looper只触发，不收真正有用的消息

				return -1;
			}
		}
	} while (0);
	return -1;
}

bool Looper::canQuit()
{
	assert(mLooperData->mBMQuit);
	#if 0
	auto& events = mLooperData->mExitEvents;
	while (events.size())
	{
		#ifdef _MSC_VER
		BOOL waitAll = TRUE;
		HANDLE arr[MAXIMUM_WAIT_OBJECTS];
		if (events.size() <= COUNT_OF(arr))
		{
			for (UINT i = 0; i < events.size(); i++)
			{
				arr[i] = events[i]->operator HANDLE();
			}

			int ret = WaitForMultipleObjects((DWORD)events.size(), arr, waitAll, 0);
			if (ret == WAIT_OBJECT_0)
			{
				events.clear();
				break;
			}
			else
			{
				return false;
			}
		}
		else
		{
			for (UINT i = 0; i < events.size(); i++)
			{
				bool signal = events[i]->Wait(0);
				//LogV(TAG,"events[%d],signal=%d",i,signal);
				if (signal)
				{
				}
				else
				{
					return false;
				}
			}

			events.clear();
		}
		#else
		for (UINT i = 0; i < events.size(); i++)
		{
			bool signal = events[i]->Wait(0);
			//LogV(TAG,"events[%d],signal=%d",i,signal);
			if (signal)
			{
			}
			else
			{
				return false;
			}
		}

		events.clear();
		#endif
	}

	{
		ULONGLONG tick = mLooperData->mTickStartQuit;
		ULONGLONG tickNow = ShellTool::GetTickCount64();
		int ms = 3000;
		if (tickNow >= tick + ms && tickNow >= mLooperData->mTickDump + ms)
		{
			mLooperData->mTickDump = tickNow;

			LogV(TAG, "after emit quit %d ms,following handlers still alive:", ms);
			mInternalData->Dump(0);//dump which object are still live
		}
	}
	//有未决事务时，应该等到所有事务完结后才能安全退出looper
	auto nc = mInternalData->GetLiveChildrenCount();
	return nc == 0;

	#endif

	return true;
}

void Looper::assertLooper()
{
	assert(isSelfThread());
}

void Looper::onCreate()
{
}

void Looper::start()
{
	run();
}

int Looper::run()
{
	while (1)
	{
		singleStep();

		if (mLooperData->mBMQuit && canQuit())
		{
			AutoLock lock(mLooperData->mMutex);
			if (mLooperData->mMessageList.size() == 0)
			{
				mLooperData->mLooperRunning = false;
				break;
			}
		}
	}

	return 0;
}

void Looper::singleStep()
{
	#if defined _CONFIG_PROFILER
	if (mEnableProfiler)
	{
		mProfiler->totalSteps++;
	}
	#endif

	tagLooperMessage msg;
	int ret = getMessage(msg);
	if (ret == 0)
	{
		/*
		注意:这里不能用ret=dispatchMessage(msg)
		message处理结果已经通过其他方式返回给message发送方
		*/

		dispatchMessage(msg);
	}
}

int64_t Looper::dispatchMessage(tagLooperMessage& msg)
{
	LRESULT ret = -1;
	if (msg.handler)
	{
		ret = msg.handler->onMessage(msg.cmd, msg.wp, msg.lp);
	}
	else
	{
		ret = onThreadMessage(msg);
	}

	if (msg.done)
	{
		if (msg.ack)
		{
			*msg.ack = ret;
		}

		*msg.done = true;

		/* 由于senderLooper是采用sendMessage调用到此,能保证senderLooper到此一直有效 */
		if (msg.waitAck && msg.sendLooper)
		{
			assert(msg.sendLooper != this);
			msg.sendLooper->wakeup();
		}
	}

	return ret;
}

int64_t Looper::onThreadMessage(tagLooperMessage& msg)
{
	assert(!msg.handler);

	return onMessage(msg.cmd, msg.wp, msg.wp);
}

int Looper::wakeup()
{
	postMessage(BM_NULL);
	return 0;
}

int64_t Looper::postMessage(uint32_t cmd, int64_t wp, int64_t lp)
{
	return postMessage(shared_from_this(), cmd, wp, lp);
}

int64_t Looper::sendMessage(uint32_t cmd, int64_t wp, int64_t lp)
{
	auto ret = sendMessage(shared_from_this(), cmd, wp, lp);
	return ret;
}

int64_t Looper::postMessage(shared_ptr<Handler>handler, int32_t msg, int64_t wp, int64_t lp)
{
	if (!mLooperData->mLooperRunning && !mLooperData->mAttachThread)
	{
		if (handler)
		{
			//LogW(TAG, "skip postMessage(handler=%s,msg=%d)", handler->GetObjectName().c_str(), msg);
		}

		//assert(FALSE);
		return 0;
	}

	tagLooperMessage loopMsg;
	loopMsg.handler = handler;
	loopMsg.cmd = msg;
	loopMsg.wp = wp;
	loopMsg.lp = lp;

	{
		if (mLooperData->mAttachThread)
		{
			/* stack looper没有消息循环,所以只触发，不加空消息 */
			if (msg != BM_NULL)
			{
				assert(FALSE);
			}
		}
		else
		{
			AutoLock lock(mLooperData->mMutex);
			if (!mLooperData->mLooperRunning)
			{
				return -1;
			}
			mLooperData->mMessageList.push_back(loopMsg);
		}

		postQueuedCompletionStatus(mLooperHandle);
	}

	return 0;
}

bool Looper::postQueuedCompletionStatus(HANDLE handle, int32_t bytes, void* key , LPOVERLAPPED overlapped)
{
	auto h = (handle == INVALID_HANDLE_VALUE) ? mLooperHandle : handle;

	bool ok = !!::PostQueuedCompletionStatus(h, (DWORD)bytes, (ULONG_PTR)key, overlapped);
	if (!ok)
	{
		logW(mTag)<<"fail to PostQueuedCompletionStatus,error="<<lastError();
		int x = 0;
		assert(false);
	}

	return ok;
}

int64_t Looper::sendMessage(shared_ptr<Handler> handler, int32_t msg, int64_t wp, int64_t lp)
{
	if (!mLooperData->mLooperRunning)
	{
		//assert(FALSE);
		logW(mTag)<<"looper is NOT running,skip msg"<<msg;
		return 0;
	}

	if (mLooperData->mAttachThread && isSelfThread())
	{
		/* 清理stack looper收到的所有BM_NULL消息 */
		AutoLock lock(mLooperData->mMutex);
		auto& msgList = mLooperData->mMessageList;
		while (!msgList.empty())
		{
			tagLooperMessage& msg = msgList.front();
			msgList.pop_front();
			if (msg.cmd == BM_NULL)
			{
				//int x = 0;
			}
			else
			{
				assert(FALSE);
			}
		}
	}

	bool done = false;
	LRESULT ack = 0;/* 默认不能为-1,原因是app可能把返回值当指针 */
	tagLooperMessage loopMsg;
	loopMsg.handler = handler;
	loopMsg.cmd = msg;
	loopMsg.wp = wp;
	loopMsg.lp= lp;
	loopMsg.done = &done;
	loopMsg.ack = &ack;

	/*
	注意:
	在主线程调用looper Start()之后马上调用sendMessage时,有可能looper中的mThreadId在_WorkThread中还没初始化，但这不影响结果
	*/

	if (isSelfThread())
	{
		return dispatchMessage(loopMsg);
	}

	loopMsg.waitAck = true;

	{
		Looper* looper = currentLooper();
		if (looper)
		{
			sendMessageHelper(loopMsg, *looper);
			return ack;
		}
	}

	#if defined MSC_VER && !defined _DEBUG
	//Looper和win32 thread互相send message时容易死锁,所以要用postMessage
	assert(FALSE);//please use postMessage
	return 0;
	#endif

	_StackLooperSendMessage(loopMsg);
	return ack;
}

class SmartTlsLooperManager
{
	static void clear();
public:
	shared_ptr<Looper> PopTlsLooper()
	{
		{
			AutoLock lock(mCS);
			if (!mLoopers.empty())
			{
				auto obj = mLoopers.back();
				mLoopers.pop_back();
				return obj;
			}
		}

		{
			static bool first = true;
			if (first)
			{
				first = false;

				atexit(SmartTlsLooperManager::clear);
			}

		}

		auto looper = make_shared<Looper>();
		looper->mThreadId = currentTid();
		looper->mLooperData->mAttachThread = true;
		looper->mLooperHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

		return looper;
	}

	void PushTlsLooper(shared_ptr<Looper> obj)
	{
		AutoLock lock(mCS);
		mLoopers.push_back(obj);
	}

protected:
	recursive_mutex mCS;
	list<shared_ptr<Looper>> mLoopers;
};

static SmartTlsLooperManager gTlsLooperManager;

void SmartTlsLooperManager::clear()
{
	AutoLock lock(gTlsLooperManager.mCS);
	gTlsLooperManager.mLoopers.clear();
}

class SmartTlsLooper
{
public:
	SmartTlsLooper(Looper* obj)
	{
		mLooper = gTlsLooperManager.PopTlsLooper();
		mLooper->mThreadId = currentTid();

		mLooperImpl = obj;
		mLooperImpl->setCurrentLooper(mLooper.get());
	}

	~SmartTlsLooper()
	{
		gTlsLooperManager.PushTlsLooper(mLooper);
		mLooperImpl->setCurrentLooper(nullptr);
	}

	Looper* mLooperImpl;
	shared_ptr<Looper> mLooper;
};

void Looper::_StackLooperSendMessage(tagLooperMessage& loopMsg)
{
	SmartTlsLooper obj(this);
	sendMessageHelper(loopMsg, *obj.mLooper.get());
}

void Looper::sendMessageHelper(tagLooperMessage& msg, Looper& looper)
{
	msg.sendLooper = &looper;

	{
		AutoLock lock(mLooperData->mMutex);
		if (!mLooperData->mLooperRunning)
		{
			return;
		}
		mLooperData->mMessageList.push_back(msg);
	}

	postQueuedCompletionStatus();
	/*
	#ifdef _DEBUG
		string info;
		info = StringTool::Format("%s,msg=%d", __func__, msg.mMsg);
		TickDumper check(info.c_str(), false, 100);
	#else
		TickDumper check(__func__, false, 100);
	#endif
	*/
	while (!*msg.done)
	{
		looper.singleStep();
	}
}

int Looper::processTimer(uint32_t & cmsDelayNext, int64_t ioIdleTick)
{
	if (mTimerManager == nullptr)
	{
		return -1;
	}

	bool needWait = true;
	auto timerMan = Looper::GetTimerManager();
	#if defined _CONFIG_PROFILER

	//避免在下面的timer中修改mEnableProfiler
	const auto enableProfiler = mEnableProfiler;

	ULONGLONG tick = 0;
	if (enableProfiler)
	{
		tick = ShellTool::GetTickCount64();
	}
	#endif

	needWait = (timerMan->processTimer(cmsDelayNext, ioIdleTick) == 0);

	#if defined _CONFIG_PROFILER
	if (enableProfiler)
	{
		tick = ShellTool::GetTickCount64() - tick;

		if (mEnableProfiler)
		{
			if (tick > mProfiler->timerMaxTick)
			{
				mProfiler->timerMaxTick = tick;
			}

		}
	}
	#endif

	if (needWait)
	{
		if (cmsDelayNext != INFINITE)
		{
			cmsDelayNext = max(1, cmsDelayNext);/* 发现timer不是很精确,不能恰好等那么长时间，也许可提高精度 */
		}

		assert(cmsDelayNext);//如果为0，会形成busy loop,占用大量cpu
	}

	return needWait ? -1 : 0;
}

shared_ptr<TimerManager> Looper::GetTimerManager()
{
	if (mTimerManager == nullptr)
	{
		mTimerManager = make_shared<TimerManager>();
	}

	return mTimerManager;
}

bool Looper::isRunning()const
{
	return mLooperData->mLooperRunning;
}

int Looper::postQuitMessage(long exitCode)
{
	if (!isRunning())
	{
		return 0;
	}

	postMessage(BM_QUIT, exitCode);
	return 0;
}

void Looper::killTimer(Timer_t& timerId)
{
	killTimer(this, timerId);
}

Timer_t Looper::setTimer(Timer_t& timerId, uint32_t interval)
{
	if (timerId)
	{
		killTimer(timerId);
	}

	return timerId = setTimer(this, interval);
}

Timer_t Looper::setTimerEx(uint32_t interval, shared_ptr<tagTimerExtraInfo> info)
{
	return setTimerEx(this, interval, info);
}

Timer_t Looper::setTimer(Handler* handler, uint32_t interval)
{
	return setTimerEx(handler, interval);
}

void Looper::killTimer(Handler* handler, Timer_t& timerId)
{
	if (!handler)
	{
		assert(FALSE);
		return;
	}

	if (!isSelfThread())
	{
		assert(FALSE);
		return;
	}

	auto timerMan = Looper::GetTimerManager();
	timerMan->killTimer(handler, timerId);
}

Timer_t Looper::setTimerEx(Handler* handler, uint32_t interval, shared_ptr<tagTimerExtraInfo> info)
{
	if (!handler)
	{
		assert(FALSE);
		return 0;
	}

	if (!isSelfThread())
	{
		assert(FALSE);
		return 0;
	}

	if (!handler->isCreated())
	{
		logW(mTag)<< "fail set timer,handler is NOT created";
		return 0;
	}

	if (!isLooper())
	{
		if (handler->isDestroyed())
		{
			if (mLooperData->mDestroyedHandlers.find(handler) == mLooperData->mDestroyedHandlers.end())
			{
				logW(mTag)<< "fail set timer,handler is destroyed";
				return 0;
			}
		}
	}

	auto timerMan = GetTimerManager();
	auto timerId = handler->mInternalData->NextTimerId();
	timerMan->setTimer(handler, timerId, interval, info);
	postMessage(BM_NULL); /* 投递消息保证重新计算等待时间 */
	return timerId;
}

Timer_t Looper::setTimer(shared_ptr<Handler>handler, uint32_t interval)
{
	return setTimerEx(handler, interval);
}

void Looper::killTimer(shared_ptr<Handler>handler, Timer_t& timerId)
{
	if (!handler)
	{
		return;
	}

	killTimer(handler.get(), timerId);
}

Timer_t Looper::setTimerEx(shared_ptr<Handler>handler, uint32_t interval, shared_ptr<tagTimerExtraInfo> info)
{
	if (!handler)
	{
		assert(FALSE);
		return 0;
	}

	return setTimerEx(handler.get(), interval, info);
}

}