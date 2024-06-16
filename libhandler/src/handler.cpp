#include "pch.h"
#include "handler.h"
#include "loger.h"
#include "looper.h"
#include "system.h"
#include "handlerdata.h"

namespace Bear {

Handler::Handler()
{
	mInternalData = make_unique<tagHandlerData>(this);
}

Handler::~Handler()
{
	mInternalData->mHandler = nullptr;

	{
		auto parent = mInternalData->mParent;
		if (parent)
		{
			parent->sendMessage(BM_CHILD_DTOR, (int64_t)this);
		}
	}
}

void Handler::onCreate()
{
	mInternalData->mOnCreateCalled = true;
}

void Handler::markDestroyed()
{
	mInternalData->mDestroy = true;
}

void Handler::onDestroy()
{
	assertLooper();
	mInternalData->mTickDestroy = tickCount();
	mInternalData->mOnDestroyCalled = true;

	markDestroyed();

	for (auto iter = mInternalData->mChildren.begin(); iter != mInternalData->mChildren.end();)
	{
		auto child = iter->second.lock();
		if (child)
		{
			if (child->isLooper())
			{
				auto looper = dynamic_pointer_cast<Looper>(child);
				looper->postQuitMessage();
			}
			else
			{
				child->sendMessage(BM_DESTROY);//这里不能使用postMessage,原因:不能保证在处理post message时的child有效性
			}

			++iter;
		}
		else
		{
			auto iterSave = iter;
			++iter;
			mInternalData->mChildren.erase(iterSave);
		}
	}

	if (isLooper())
	{
		//looper mSelfRef是由looper thread entry特殊处理的
		//do nothing here
	}
	else
	{
		if (mInternalData->mPassive && mInternalData->mSelfRef)
		{
			postDispose(mInternalData->mSelfRef);
		}

		mInternalData->mLooper->sendMessage(BM_HANDLER_DESTROY, (WPARAM)this);

	}
}

bool Handler::isSelfLooper()const
{
	auto tid = currentTid();
	return mThreadId == tid;
}

int64_t Handler::onMessage(uint32_t msg, int64_t wp, int64_t lp)
{
	switch (msg)
	{
	case BM_NULL:
	{
		return 0;
	}
	case BM_CREATE:
	{
		if (isCreated())
		{
			logW(mTag)<<"skip double BM_CREATE";
		}
		else
		{
			mInternalData->mCreated = true;
			onCreate();
			if (!mInternalData->mOnCreateCalled)
			{
				logW(mTag)<< getName()<<" make sure __super::OnCreate() is called";
				assert(false);
			}
		}

		return 0;
	}
	case BM_DESTROY:
	{
		//logV(mTag) << "BM_DESTROY,this=" << this;
		if (!mInternalData->mDestroyMarkCalled)
		{
			mInternalData->mDestroyMarkCalled = true;
			onDestroy();

			//当用户忘记调用基类OnDestroy()时给出提醒
			if (!mInternalData->mOnDestroyCalled)
			{
				logW(mTag)<<getName()<<" make sure __super::OnDestroy() is called";
				assert(false);
			}
		}
		return 0;
	}

	/*
	2023.02.02增加,在child handler析构时主动删除mChildren中的weak_ptr
	原因:
	理想情况下BigObject占用的内存在最后一个shared_ptr失效时释放
	控制块在weak_ptr失效时释放

	现在用make_shared时控制块和BigObject是一起分配的，
	当shared_ptr失效时，只调用了BigObject的析构
	当weak_ptr失效时才释放了控制块和BigObject占用的内存
	//*/
	case BM_CHILD_DTOR:
	{
		long* item = (long*)wp;
		auto& items = mInternalData->mChildren;
		auto it = items.find(item);
		if (it != items.end())
		{
			items.erase(it);
		}

		return 0;
	}
	/*
	case BM_DUMP:
	{
		mInternalData->Dump((int)(long)wp, (bool)lp);
		return 0;
	}
	case BM_DUMP_ALL:
	{
		mInternalData->DumpAll();
		return 0;
	}
	*/
	case BM_ADD_CHILD:
	{
		weak_ptr<Handler>* child = (weak_ptr<Handler>*)wp;
		//const char* name = (const char*)lp;
		return mInternalData->addChildHelper(*child);
	}
	/*
	case BM_FIND_OBJECT:
	{
		tagFindObjectInfo& info = *(tagFindObjectInfo*)wp;
		string& url = *(string*)lp;
		info.mHandler = mInternalData->FindObject_Impl(url);
		return 0;
	}
	case BM_GET_CHILD:
	{
		tagFindObjectInfo& info = *(tagFindObjectInfo*)wp;
		LONG_PTR id = (LONG_PTR)lp;
		info.mHandler = mInternalData->GetChild_Impl(id);
		return 0;
	}
	case BM_GET_SHORTCUT:
	{
		tagGetShortcut& info = *(tagGetShortcut*)wp;
		const string& url = *(string*)lp;
		info.mHandler = mInternalData->Shortcut_Impl(url);
		return 0;
	}
	case BM_REGISTER_SHORTCUT:
	{
		auto name = (string*)wp;
		auto obj = (weak_ptr<Handler>*)lp;
		return mInternalData->RegisterShortcut_Impl(*name, *obj);
	}

	case BM_PRE_EXECUTE:
	{
		auto obj = dynamic_pointer_cast<AsyncTask>(shared_from_this());
		if (obj)
		{
			obj->OnPreExecute();
		}
		return 0;
	}
	case BM_POST_EXECUTE:
	{
		auto obj = dynamic_pointer_cast<AsyncTask>(shared_from_this());
		if (obj)
		{
			obj->OnPostExecute();
		}
		return 0;
	}
	*/
	case BM_REMOVE_CHILD_WEAKREF:
	{
		Handler* handler = (Handler*)wp;
		mInternalData->RemoveChildWeakRef_Impl(handler);
		return 0;
	}
	case BM_POST_RUNNABLE:
	{
		auto info = (tagDelayedRunnable*)wp;
		if (info->mDelayedMS == 0)
		{
			info->mRunnable->Run();
		}
		else
		{
			auto infoEx = make_shared<tagTimerExtraInfo>();
			infoEx->mRunnable = info->mRunnable;
			mInternalData->SetTimerEx(info->mDelayedMS, infoEx);
		}
		info->mSelfRef = nullptr;
		return 0;
	}
	case BM_CANCEL_RUNNABLE:
	{
		auto info = (tagCancelRunnable*)wp;
		{
			Looper::currentLooper()->cancelRunnableInQueue(info->mHandler, info->mRunnable);

			auto& timerMap = mInternalData->mTimerMap;
			if (timerMap)
			{
				vector<Timer_t> timerIds;
				for (auto iter = timerMap->begin(); iter != timerMap->end(); ++iter)
				{
					auto item = iter->second->mExtraInfo;
					if (item && item->mRunnable == info->mRunnable)
					{
						timerIds.push_back(iter->first);
					}
				}

				for (auto iter = timerIds.begin(); iter != timerIds.end(); ++iter)
				{
					//LogW(TAG,"cancel runnable,id=%d",*iter);
					killTimer(*iter);
				}
			}
		}

		return 0;
	}
	case BM_SEND_RUNNABLE_FUNCTIONAL:
	{
		auto pfn = (std::function<void()>*)wp;
		(*pfn)();

		return 0;
	}
	/*
	case BM_MESSAGE:
	{
		auto& obj = *(shared_ptr<Message>*)wp;
		HandleMessage(obj);
		return 0;
	}
	case BM_POST_MESSAGE:
	{
		auto p = (tagSelfRef*)wp;
		auto obj = p->mSelfRef;
		p->mSelfRef = nullptr;

		auto msg = dynamic_pointer_cast<Message>(obj->mObject);
		HandleMessage(msg);
		return 0;
	}
	*/
	default:
	{
		break;
	}
	}

	return 0;
}

void Handler::setObjectName(const string& name)
{
	assertLooper();
	mInternalData->SetName(name);
}

void Handler::assertLooper()const
{
	if (mThreadId == 0) {
		return ;
	}

	assert(isSelfLooper());
}

string Handler::getName()const
{
	//assertLooper();
	return mInternalData->GetName();
}

bool Handler::isCreated()const
{
	return mInternalData->mCreated;
}

bool Handler::isDestroyed()const
{
	return mInternalData->mDestroy;
}

shared_ptr<Handler> Handler::parent()const
{
	return mInternalData->mParent;
}

bool Handler::isLooper()const
{
	return mInternalData->mIsLooper;
}

int64_t Handler::sendMessage(uint32_t msg, int64_t wp, int64_t lp)
{
	if (!mInternalData->mCreated && msg != BM_CREATE)
	{
		logW(mTag)<< "fail sendMessage,need call Create()";
		return -1;
	}

	if (!mInternalData->mLooper)
	{
		logW(mTag)<< "looper is null,skip msg="<<msg<<",name="<<getName();
		return 0;
	}

	return mInternalData->mLooper->sendMessage(shared_from_this(), msg, wp, lp);
}

//parent对child作弱引用
//child对parent作强引用
int Handler::addChild(weak_ptr<Handler> child)
{
	if (isSelfLooper())
	{
		return mInternalData->addChildHelper(child);
	}

	syncSend([this,child]() {
		return mInternalData->addChildHelper(child);
			 });
	return 0;
	//return (int)sendMessage(BM_ADD_CHILD, (WPARAM)&child, (LPARAM)name.c_str());
}

void Handler::asyncPost(const function<void()>& fn)
{

}

void Handler::syncSend(const function<void()>& fn)
{
	if (isSelfLooper())
	{
		fn();
	}
	else
	{
		sendMessage(BM_SEND_RUNNABLE_FUNCTIONAL, (int64_t)&fn);
	}
}

//避免在.h文件中引用Loop接口
int Handler::postDisposeHelper(uint64_t wp)
{
	if (isCreated())
	{
		mInternalData->mLooper->postMessage(BM_POST_DISPOSE, (WPARAM)wp);
		return 0;
	}
	else
	{
		logW(mTag)<< "fail "<< __func__;
	}

	return -1;
}

Timer_t Handler::setTimer(Timer_t& timerId, uint32_t interval)
{
	if (timerId)
	{
		killTimer(timerId);
	}

	return timerId = mInternalData->setTimer(interval);
}

void Handler::killTimer(Timer_t& timerId)
{
	mInternalData->killTimer(timerId);
}

//说明:
//对Handler来说,主动调用create()没太大意义,并且很容易忘记调用它
//所以约定BM_CREATE是可选的
//考虑到一般都会调用addChild,所以在addChild()中会适时触发BM_CREATE
//注意有可能跨looper来调用create或addChild,所以BM_CREATE都采用sendMessage来触发
int Handler::create(Handler* parent)
{
	shared_ptr<Handler> parentObj;
	if (parent)
	{
		parentObj = parent->shared_from_this();
	}
	auto ret = -1;
	if (parentObj)
	{
		ret = parentObj->addChild(shared_from_this());
	}

	if (ret == 0 && !mInternalData->mCreated)
	{
		sendMessage(BM_CREATE);
	}

	if (!mInternalData->mCreated)
	{
		logW(mTag) << __func__ << " fail";
	}

	return mInternalData->mCreated ? 0 : -1;
}

void Handler::destroy()
{
	if (!isCreated())
	{
		return;
	}

	if (!mInternalData->mDestroy)
	{
		mInternalData->mDestroy = true;
		sendMessage(BM_DESTROY);
	}
}

}
