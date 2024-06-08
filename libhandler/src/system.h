#pragma once
#include "timems.h"

namespace Bear {

int64_t tickCount();
int currentPid();
int currentTid();
tagTimeMs currentTimeMs();

}