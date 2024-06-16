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
static const char* TAG = "looper";
Looper::Looper()
{
	mTag = TAG;
	mLooperData = make_unique<tagLooperData>(this);
	mInternalData->mIsLooper = true;
	mInternalData->SetActiveObject();
	mLooperTick = tickCount();

	logV(mTag) << __func__ << ",this=" << this;
}

Looper::~Looper()
{
	logV(mTag) << __func__ << ",this=" << this;
	if (getMainLooper() == this)
	{
		setMainLooper(nullptr);
	}

	if (mLooperData->mStarted && currentLooper())
	{
		if (currentLooper() == this)
		{
			//DV("%s is current looper,reset it", GetObjectName().c_str());
			setCurrentLooper(nullptr);
		}
		else
		{
			if (mLooperData->mAttachThread)
			{
				logW(mTag) << "cross thread destroy TLS looper,maybe left obsolete raw pointer,it is very dangerous!";
			}

			logW(mTag) << getName()<< " is NOT current looper "<< currentLooper();
		}
	}

	LONGLONG tick = 0;
	if (mLooperData->mTickStartQuit)
	{
		tick = tickCount() - mLooperData->mTickStartQuit;
	}

	logV(mTag)<<__func__<< "this="<<this<<",quit tick="<<tick;

	assert(!mLooperData->mLooperRunning);

	if (mLooperHandle != (HANDLE)INVALID_HANDLE_VALUE)
	{
		CloseHandle(mLooperHandle);
		mLooperHandle = (HANDLE)INVALID_HANDLE_VALUE;
	}

	auto& msgList = mLooperData->mMessageList;
	bool messageEmpty = msgList.empty();

	if (!messageEmpty)
	{
		bool fatalError = true;
		auto nc = msgList.size();
		tagLooperMessage msg;
		if (nc == 1)
		{
			msg = msgList.front();
			if (msg.cmd == BM_NULL)
			{
				fatalError = false;
			}
		}

		if (fatalError)
		{
			logW(mTag)<< "fatal error,message pending in queue,please check app logic,nc="<< nc;
			if (!mLooperData->mAttachThread)
			{
				assert(FALSE);
			}
		}
	}

}

static Looper* gMainLooper = nullptr;
Looper* Looper::getMainLooper()
{
	return gMainLooper;
}

bool Looper::isMainLooper(Looper* looper)
{
	return gMainLooper == looper;
}

int Looper::setMainLooper(Looper* looper)
{
	logV(TAG)<< __func__<<" looper = "<<looper;
	if (looper)
	{

		if (gMainLooper)
		{
			assert(false);
			return -1;
		}

	}

	gMainLooper = looper;
	return 0;
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
	{
		ULONGLONG tick = mLooperData->mTickStartQuit;
		ULONGLONG tickNow = tickCount();
		int ms = 3000;
		if (tickNow >= tick + ms && tickNow >= mLooperData->mTickDump + ms)
		{
			mLooperData->mTickDump = tickNow;

			logV(mTag)<< "after emit quit "<<ms<<" ms,some handlers still alive";
			//mInternalData->Dump(0);//dump which object are still live
		}
	}
	#endif

	//有未决事务时，应该等到所有事务完结后才能安全退出looper
	auto nc = mInternalData->GetLiveChildrenCount();
	if (nc == 0)
	{
		int x = 0;
	}
	return nc == 0;


	return true;
}

void Looper::assertLooper()
{
	assert(isSelfLooper());
}

void Looper::onCreate()
{
	__super::onCreate();
}

void Looper::start()
{
	if (mLooperData->mStarted)
	{
		assert(false);
		return ;
	}

	bool newThread = true;
	startHelper(newThread);
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
				//logV(mTag) << " exit run(),this="<<this;
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
	if (mLooperData->mStarted &&  !mLooperData->mLooperRunning)
	{
		logW(mTag)<<"looper is NOT running,skip msg "<<msg;
		return 0;
	}

	const auto selfLooper = isSelfLooper();
	if (mLooperData->mAttachThread && selfLooper)
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

	if (selfLooper)
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

int Looper::postQuitMessage(int32_t exitCode)
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

	if (!isSelfLooper())
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

	if (!isSelfLooper())
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
	auto timerId = ++handler->mInternalData->mNextTimerId;// NextTimerId();
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

void* Looper::_WorkThreadCB(void* p)
{
	auto pThis = (Looper *)p;
	gBaseLooper = pThis;

	auto ret = pThis->_WorkThread();
	return ret;
}

void* Looper::_WorkThread()
{
	//*
	if (!mThreadName.empty())
	{
		setThreadName(mThreadName);
	}
	mThreadId = currentTid();
	/*/

	//LogV(TAG,"%s::%s,mThreadId=%d", mThreadName.c_str(), __func__, mThreadId);

	/*
	说明:在创建thread之前就要置mCreated = true;
	如果到这里才置true,由于线程调度影响，可能导致looper->Start()之后mCreate短暂为false而导致竞争关系
	影响是send/post会失败!
	*/

	onCreate();

	run();

	auto ret = mLooperData->mExitCode;
	assert(!mLooperData->mLooperRunning);

	assert(mInternalData->mSelfRef);
	mInternalData->mSelfRef = nullptr;//这里会自动调用delete this

	setThreadName("UserWorkItem");

	return (void*)(int64_t)ret;
}

int Looper::startHelper(bool newThread)
{
	if (mLooperData->mLooperRunning)
	{
		logW(mTag)<< "mLooperRunning="<< mLooperData->mLooperRunning<< ",CurrentLooper="<< currentLooper();
		assert(FALSE);
		return 0;
	}

	{
		assert(mLooperHandle == INVALID_HANDLE_VALUE);
		mLooperHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	}

	mInternalData->mSelfRef = shared_from_this();

	mLooperData->mLooperRunning = true;
	mLooperData->mBMQuit = false;
	mLooperData->mStarted = true;

	if (newThread)
	{
		mInternalData->mCreated = true;
		BOOL ok = QueueUserWorkItem((LPTHREAD_START_ROUTINE)_WorkThreadCB, (LPVOID)this, 0);
		return ok ? 0 : -1;
	}

	Looper* looperCache = currentLooper();
	mInternalData->mCreated = true;
	_WorkThreadCB(this);
	setCurrentLooper(looperCache);
	return 0;
}

int64_t Looper::onMessage(uint32_t msg, int64_t wp, int64_t lp)
{
	switch (msg)
	{
	case BM_HANDLER_DESTROY:
	{
		auto handler = (Handler*)wp;
		auto obj = handler->shared_from_this();
		mLooperData->mDestroyedHandlers[handler] = obj;

		if (!mLooperData->mTimerGC)
		{
			setTimer(mLooperData->mTimerGC, 1);
		}

		auto count = mLooperData->mDestroyedHandlers.size();
		if (count > 100)
		{
			logV(mTag)<< "this="<<this<<",gc size="<<count;
		}

		return 0;
	}
	case BM_POST_DISPOSE:
	{
		auto info = (tagAutoObject*)wp;
		info->clear();
		mLooperData->gc();
		return 0;
	}

	case BM_QUIT:
	{
		if (!mLooperData->mBMQuit)
		{
			mLooperData->mExitCode = (long)wp;
			mLooperData->mTickStartQuit = tickCount();
			postMessage(BM_DESTROY);
			onBMQuit();
		}
		break;
	}
	/*
	#if defined _CONFIG_PROFILER
	case BM_PROFILER_START:
	{
		if (mEnableProfiler && mProfiler)
		{
			return 0;
		}

		if (!mProfiler)
		{
			mProfiler = new tagProfiler;
		}

		mProfiler->clear();
		mProfiler->startTick = ShellTool::GetTickCount64();
		mEnableProfiler = true;

		return 0;
	}
	case BM_PROFILER_STOP:
	{
		if (mEnableProfiler && mProfiler)
		{
			mProfiler->stopTick = ShellTool::GetTickCount64();
		}

		mEnableProfiler = false;

		//不要删除mProfiler

		return 0;
	}
	#endif
	*/

	}

	return __super::onMessage(msg, wp, lp);
}

void Looper::onBMQuit()
{
	assert(!mLooperData->mBMQuit);
	mLooperData->mBMQuit = true;
	//LogV(TAG,"%s,mBMQuit=%d",mThreadName.c_str(),mBMQuit);

	assert(mLooperData->mTimerCheckQuitLooper == 0);
	if (mLooperData->mTimerCheckQuitLooper == 0)
	{
		/*
		注意:如果CanQuitLooperNow()返回false时,并且后续动作不能主动触发looper,
		则需要主动定时触发CanQuitLooperNow()，否则looper有可能死等待
		*/
		mLooperData->mTimerCheckQuitLooper = setTimerEx(10);//定时检查CanQuitLooperNow()
	}
}

void Looper::cancelRunnableInQueue(shared_ptr<Handler>& handler, shared_ptr<Runnable>& runnable)
{
	if (!isSelfLooper())
	{
		assert(false);
		return;
	}

	{
		AutoLock lock(mLooperData->mMutex);

		auto& items = mLooperData->mMessageList;
		for (auto iter2 = items.begin(); iter2 != items.end();)
		{
			auto iter = iter2++;

			if (iter->cmd != BM_POST_RUNNABLE
				|| iter->handler != handler
				)
			{
				continue;
			}

			auto info = (tagDelayedRunnable*)iter->wp;
			if (info->mRunnable == runnable)
			{
				items.erase(iter);
				info->mSelfRef = nullptr;

				//可能存在多个，所以要继续遍历
			}
		}
	}

}

void Looper::onTimer(Timer_t id)
{
	if (id == mLooperData->mTimerGC)
	{
		mLooperData->gc();
		auto n = mLooperData->mSeqGC;
		if (n > 0)
		{
			auto interval = (n + 1) * n / 2;
			auto ms = min(1000, interval);
			setTimer(mLooperData->mTimerGC, ms);
		}
		return;
	}

	__super::onTimer(id);
}


}