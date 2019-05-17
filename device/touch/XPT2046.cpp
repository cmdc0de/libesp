
#include "XPT2046.h"

using namespace libesp;

XPT2046::XPT2046(SPIDevice *device, uint32_t measurementsToAverage, int32_t msBetweenMeasures)
	: Notificaions(), MyDevice(device), ShouldProcess(false), MeasurementsToAverage(measurementsToAverage), MSBetweenMeasurements(msBetweenMeasures) {

}
	
//set up irq -
// one for pos edge pen down
// one for neg edge pen up
ErrorType XPT2046::init(const gpio_num_t &interruptPin) {

}

//if pen irq fires ShouldProcess will be set
// until pen comes off (another interrupt) we will meausre pen position
// each position measured will measured, MeasurementsToAverage times, with a 'time between mesaures' as well.
ErrorType XPT2046::process() {

}

/*
* add / remove observers
*/
bool XPT2046::addObserver(const xQueueHandle &o) {

}

bool XPT2046::removeObserver(const xQueueHandle &o) {

}

/*
* does not try to acquire just sets pwr mode
* if the power mode turns off the pen the acquision will stay off
*/
void XPT2046::setPwrMode(uint8_t pwrMode) {

}

XPT2046::~XPT2046() {

}

