#pragma once
#include "handler.h"

namespace Bear {

class Looper :public Handler {
public:
	Looper();
protected:
	void onCreate();
};

}