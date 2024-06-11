#include "pch.h"
#include "handler.h"
#include "system.h"

namespace Bear {

Handler::Handler()
{
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

}
