#include "pch.h"
#include "system.h"
#include <thread>

#ifdef _MSC_VER
#include <windows.h>
using namespace std::this_thread;
#endif

namespace Bear {

int64_t tickCount()
{
	auto tick = (int64_t)GetTickCount64();
	return tick;

}

int currentPid()
{
	return _getpid();
}
int currentTid()
{
	return (int)GetCurrentThreadId();
}

int lastError()
{
	return GetLastError();
}

}