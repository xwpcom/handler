#pragma once

namespace Bear {

class Handler {
public:
	Handler();
	virtual ~Handler();
	
	virtual int  create();
	virtual void destroy();
	virtual int  addChild(shared_ptr<Handler> obj);

	virtual void async(function<void>());
	virtual void sync(function<void>());
	virtual Timer_t setTimer(Timer_t& id,uint32_t ms);
	virtual void killTimer(Timer_t& id);
protected:
	virtual void onCreate();
	virtual void onDestroy();
	virtual void onTimer(Timer_t id);

};

}
