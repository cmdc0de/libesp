#include "app.h"
#include "../freertos.h"

using namespace libesp;

App::App() : CurrentState(0), LastRunTime(0), LastRunPerformance(0) {

}

ErrorType App::init() {
	return onInit();
}

ErrorType App::run() {
	LastRunTime = FreeRTOS::getTimeSinceStart();
	ErrorType et = onRun();
	LastRunPerformance = FreeRTOS::getTimeSinceStart()-LastRunTime;
	return et;
}
