#include "pch.h"
#include "system.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

namespace Bear {

int64_t tickCount()
{
	auto tick = (int64_t)GetTickCount64();
	return tick;

}

}