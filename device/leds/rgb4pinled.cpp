#include "rgb4pinled.h"


using namespace libesp;

LED4Pin::LED4Pin (const RGB &c,gpio_num_t RPin, gpio_num_t GPin, gpio_num_t BPin)
	: Color(c), RedPin(RPin), GreenPin(GPin), BluePin(BPin) {

}

ErrorType LED4Pin::init(ledc_timer_config_t &t) {
	esp_err_t e = ledc_timer_config(&t);
	if(e==ESP_OK) {
		ledc_channel_config_t cc;
		cc.channel = LEDC_CHANNEL_0;
		cc.duty = t.duty_resolution*2;
		cc.gpio_num = RedPin;
		cc.timer_sel = t.timer_num;
		cc.hpoint = 0xFFFF;
		cc.speed_mode = LEDC_HIGH_SPEED_MODE;
		cc.intr_type = LEDC_INTR_DISABLE;
		e = ledc_channel_config(&cc);
		if(e==ESP_OK) {
			cc.channel = LEDC_CHANNEL_1;
			cc.gpio_num = GreenPin;
			e = ledc_channel_config(&cc);
			if(e==ESP_OK) {
				cc.channel = LEDC_CHANNEL_2;
				cc.gpio_num = BluePin;
			}
			e = ledc_channel_config(&cc);
			setColor(Color);
		}
	}
	return ErrorType(e);
}


void LED4Pin::setColor(const RGB &c) {
	Color = c;
	ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, static_cast<uint32_t>(c.getRed()), 0xFF);
	ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, static_cast<uint32_t>(c.getGreen()), 0xFF);
	ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, static_cast<uint32_t>(c.getBlue()), 0xFF);
}


