#ifndef _LIBESP_RGB4PINLED_H
#define _LIBESP_RGB4PINLED_H

#include "led_color.h"
#include "../../error_type.h"
#include <driver/ledc.h>

namespace libesp {
/*
 * Traditional 4 PIN led, Red, Green, Blue pin with common Cathode
 * Color is set by PWM of color / 255.
 */
class LED4Pin {
public:
	LED4Pin(const RGB &c);
	ErrorType init(ledc_timer_config_t &t, gpio_int_type_t RPin, gpio_int_type_t GPin, gpio_int_type_t BPin);
	void setColor(const RGB &c);
	const RGB &getColor() const {return Color;}
private:
	RGB Color;
};

}

#endif
