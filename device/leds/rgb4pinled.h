#ifndef _LIBESP_RGB4PINLED_H
#define _LIBESP_RGB4PINLED_H

#include "led_color.h"
#include "../../error_type.h"
#include <driver/ledc.h>

namespace libesp {
/*
 * Traditional 4 PIN led, Red, Green, Blue pin with common anode
 * Color is set by PWM of color / 255.
 */
class LED4Pin {
public:
	LED4Pin(const RGB &c,gpio_num_t RPin, gpio_num_t GPin, gpio_num_t BPin);
	ErrorType init(ledc_timer_config_t &t);
	void setColor(const RGB &c);
	const RGB &getColor() const {return Color;}
private:
	RGB Color;
	gpio_num_t RedPin;
	gpio_num_t GreenPin;
	gpio_num_t BluePin;
};

}

#endif
