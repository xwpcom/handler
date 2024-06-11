#pragma once
#include <memory>
#include <functional>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include <cassert>

#ifdef _MSC_VER
#include <windows.h>
#endif

namespace Bear {
using namespace std;
typedef int32_t Timer_t;

#ifdef _MSC_VER
#define SUPER(X) using X::X;
#elif defined _CONFIG_INGENIC
#define SUPER(X) private: typedef X __super;
#else
#define SUPER(X) using X::X; private: typedef X __super;
#endif

}
#include "system.h"
#include "handler.h"
#include "looper.h"
#include "loger.h"