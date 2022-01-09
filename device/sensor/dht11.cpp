#include "esp_timer.h"
#include "driver/gpio.h"
//#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../system.h"

#include "dht11.h"

using namespace libesp;


int32_t DHT11::waitOrTimeout(uint16_t microSeconds, int level) {
    int micros_ticks = 0;
    while(gpio_get_level(dht_gpio) == level) { 
        if(micros_ticks++ > microSeconds) 
            return -1;
        ets_delay_us(1);
    }
    return micros_ticks;
}

bool DHT11::checkCRC(uint8_t data[]) {
    if(data[4] == (data[0] + data[1] + data[2] + data[3]))
        return true;
    else
        return false;
}

void DHT11::sendStartSignal() {
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    ets_delay_us(20 * 1000);
    gpio_set_level(dht_gpio, 1);
    ets_delay_us(40);
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);
}

ErrorType DHT11::checkResponse() {
	ErrorType et;
    /* Wait for next step ~80us*/
    if(waitOrTimeout(80, 0) == -1)
        et = ErrorType(ErrorType::TIMEOUT_ERROR);

    /* Wait for next step ~80us*/
    if(waitOrTimeout(80, 1) == -1)
        et = ErrorType(ErrorType::TIMEOUT_ERROR);

    return et;
}

DHT11::DHT11() : dht_gpio(NOPIN), LastReading(), LastReadTime(-2000000) {

}

ErrorType DHT11::init(gpio_num_t gpin) {
  ErrorType et;
	dht_gpio = gpin;
	LastReadTime = -2000000;
  return et;
}
	
void DHT11::shutdown() {

}

DHT11::~DHT11() {
  shutdown();
}

ErrorType DHT11::read(DHT11::Data &d) {
	ErrorType et;
    /* Tried to sense too soon since last read (dht11 needs ~2 seconds to make a new read) */
    if(esp_timer_get_time() - 2000000 < LastReadTime) {
    	d = LastReading;
        return et;
    }

    LastReadTime = esp_timer_get_time();

    uint8_t data[5] = {0,0,0,0,0};

    sendStartSignal();
    et = checkResponse();

    if(et.ok()) {
		/* Read response */
		for(int i = 0; i < 40; i++) {
			/* Initial data */
			if(waitOrTimeout(50, 0) != -1) {
        int32_t time = waitOrTimeout(70, 1);
				if(time > 28) {
					/* Bit received was a 1 */
					data[i/8] |= (1 << (7-(i%8)));
				} else if (-1==time){
					et = ErrorType::TIMEOUT_ERROR;
					break;
				} else {
          //its a 0 so leave alone
        }
			} else {
				et = ErrorType::TIMEOUT_ERROR;
				break;
			}
		}

		if(et.ok()) {
			if(checkCRC(data)) {
				LastReading.temperature = data[2];
				LastReading.humidity = data[0];
				d = LastReading;
			} else {
				et = ErrorType::DEVICE_CRC_ERROR;
			}
		}
    }
    return et;
}

