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
}

void Handler::onCreate()
{
}

void Handler::onDestroy()
{

}

bool Handler::isSelfThread()const
{
	auto tid = currentTid();
	return mThreadId == tid;
}

int64_t Handler::onMessage(uint32_t msg, int64_t wp, int64_t lp)
{
	return 0;
}

void Handler::setObjectName(const string& name)
{
	mInternalData->SetName(name);
}

string Handler::getName()const
{
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
		logW(mTag)<< "mInternalData->mLooper is null,msg="<<msg<<",name="<<getName();
		return 0;
	}

	return mInternalData->mLooper->sendMessage(shared_from_this(), msg, wp, lp);
}


}
