#pragma once
#include "object.h"

namespace Bear {

//约定:
//采用LOOPER_SAFE修饰的接口可以安全的跨looper调用
//没有采用LOOPER_SAFE修饰的接口，不保证跨looper安全调用，应该只在handler所在looper里调用
#define LOOPER_SAFE	//表示handler创建后接口是跨looper安全的

/*
XiongWanPing 2024.05.30

*/
class Handler :public Object
	,public enable_shared_from_this<Handler>
{
	friend class Looper;
	friend class LooperImpl;
	friend class Looper_Windows;
	friend class Looper_Linux;
	friend class TimerManager;
	friend class tagHandlerInternalData;

public:
	Handler();
	virtual ~Handler();
	
	virtual void create() {}
	virtual void destroy() {}
	virtual int addChild(weak_ptr<Handler> obj) { return -1; }
	shared_ptr<Handler> parent();
	bool isLooper()const;

	virtual void asyncPost(function<void>()) {}
	virtual void syncSend(function<void>()) {}
	virtual Timer_t setTimer(Timer_t& id, uint32_t ms) { return 0; }
	virtual void killTimer(Timer_t& id) {}
protected:
	virtual void onCreate();
	virtual void onDestroy();
	virtual void onTimer(Timer_t id) {}

	bool isCreated()const;
	bool isDestroyed()const;

	int64_t mThreadId = 0;

	void* operator new(size_t) = delete; //disable new,please use make_shared
	typedef int64_t* (Handler::* PFN_OnMessage)(int64_t* wp, int64_t* lp);
};

}
