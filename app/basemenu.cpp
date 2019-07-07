
#include <string.h>
#include "basemenu.h"
#include "../freertos.h"

using namespace libesp;

BaseMenu::BaseMenu() :
		StateData(0), TimesRunCalledAllTime(0), TimesRunCalledSinceLastReset(0), StateStartTime(0) {
}

BaseMenu::ReturnStateContext BaseMenu::run() {
	++TimesRunCalledAllTime;
	ReturnStateContext sr(this);
	if (!hasBeenInitialized()) {
		TimesRunCalledSinceLastReset = 0;
		ErrorType et = init();
		if (!et.ok()) {
			sr.NextMenuToRun = 0;
			sr.Err = et;
		}
	} else {
		++TimesRunCalledSinceLastReset;
		sr = onRun();
		if (sr.NextMenuToRun != this) {
			shutdown();
		}
	}
	return sr;
}

BaseMenu::~BaseMenu() {
}

ErrorType BaseMenu::init() {
	ErrorType et = onInit();
	if (et.ok()) {
		setState(INIT_BIT);
		StateStartTime = FreeRTOS::getTimeSinceStart();
	}
	return et;
}
ErrorType BaseMenu::shutdown() {
	ErrorType et = onShutdown();
	clearState(INIT_BIT);
	StateStartTime = 0;
	return et;
}

uint32_t BaseMenu::timeInState() {
	return FreeRTOS::getTimeSinceStart() - StateStartTime;
}

