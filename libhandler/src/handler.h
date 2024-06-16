#pragma once
#include "object.h"
#include "loger.h"
#include "timerextrainfo.h"

namespace Bear {
struct tagHandlerData;
//约定:
//采用LOOPER_SAFE修饰的接口可以安全的跨looper调用
//没有采用LOOPER_SAFE修饰的接口，不保证跨looper安全调用，应该只在handler所在looper里调用
#define LOOPER_SAFE	//表示handler创建后接口是跨looper安全的

/*
XiongWanPing 2024.05.30

*/
class Handler :public Object
	, public enable_shared_from_this<Handler>
{
public:
	Handler();
	virtual ~Handler();

	virtual int create(Handler* parent);
	virtual void destroy();
	virtual int addChild(weak_ptr<Handler> obj);
	shared_ptr<Handler> parent()const;
	bool isLooper()const;

	void LOOPER_SAFE setObjectName(const string& name);
	string getName()const;
	virtual void LOOPER_SAFE asyncPost(const function<void()>&);
	virtual void LOOPER_SAFE syncSend(const function<void()>&);
	bool LOOPER_SAFE isSelfLooper()const;
	void assertLooper()const;

	//延迟清除对象，可避免提前清除当前调用栈中还在引用的对象
	template<class T> void LOOPER_SAFE postDispose(shared_ptr<T>& sp)
	{
		if (!isCreated())
		{
			logW("Handler") << "fail " << __func__;
			return;
		}
		if (!sp)
		{
			return;
		}

		auto obj = dynamic_pointer_cast<Handler>(sp);
		if (obj)
		{
			if (obj->isDestroyed())
			{
				//do nothing
			}
			else
			{
				obj->markDestroyed();
				obj->sendMessage(BM_DESTROY);
			}
		}

		auto info = make_shared<tagPostDisposeInfo<T>>();
		info->mSelfRef = info;
		info->mSmartPtr.swap(sp);
		auto ret = postDisposeHelper((WPARAM)info.get());
		if (ret)
		{
			info->mSelfRef = nullptr;
		}
	}
protected:
	virtual void onCreate();
	virtual void onDestroy();
	virtual Timer_t setTimer(Timer_t& id, uint32_t ms);
	virtual void killTimer(Timer_t& id);
	virtual int64_t onMessage(uint32_t msg, int64_t wp = 0, int64_t lp = 0);
	virtual int64_t sendMessage(uint32_t msg, int64_t wp=0, int64_t lp = 0);
	virtual void onTimer(Timer_t id) {}
	
	int postDisposeHelper(uint64_t wp);

	bool isCreated()const;
	bool isDestroyed()const;
	void markDestroyed();

	void* operator new(size_t) = delete; //disable new,please use make_shared
	//typedef int64_t* (Handler::* PFN_OnMessage)(int64_t* wp, int64_t* lp);

	string mTag="handler";
private:
	int64_t mThreadId = 0;
	shared_ptr<tagHandlerData> mInternalData;

	friend class Looper;
	friend class TimerManager;
	friend struct tagHandlerData;
	friend struct tagLooperData;
	friend class SmartTlsLooper;
	friend class SmartTlsLooperManager;
};

}
