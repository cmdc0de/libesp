#include "rgb4pinled.h"


using namespace libesp;

LED4Pin::LED4Pin(const RGB &c) : Color(c) {}

ErrorType LED4Pin::init(ledc_timer_config_t &t, gpio_int_type_t RPin, gpio_int_type_t GPin, gpio_int_type_t BPin) {
	return ErrorType();
}


void LED4Pin::setColor(const RGB &c) {

}


