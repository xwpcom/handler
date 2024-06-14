#pragma once
#include "timems.h"

namespace Bear {

int64_t tickCount();
int currentPid();
int currentTid();
tagTimeMs currentTimeMs();
int lastError();
void setThreadName(const string& name, int dwThreadID=-1);

}