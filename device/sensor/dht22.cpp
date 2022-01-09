#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../system.h"

#include "dht22.h"

using namespace libesp;
/*
 Copy/paste from AM2302/DHT22 Docu:

DATA: Hum = 16 bits, Temp = 16 Bits, check-sum = 8 Bits

Example: MCU has received 40 bits data from AM2302 as 0000 0010 1000 1100 0000 0001 0101 1111 1110 1110 16 bits RH data + 16 bits T data + check sum
    convert 16 bits RH data from binary system to decimal system, 0000 0010 1000 1100 → 652 Binary system Decimal system: RH=652/10=65.2%RH
    convert 16 bits T data from binary system to decimal system, 0000 0001 0101 1111 → 351 Binary system Decimal system: T=351/10=35.1°C

When highest bit of temperature is 1, it means the temperature is below 0 degree Celsius. Example: 1000 0000 0110 0101, T= minus 10.1°C: 16 bits T data
    Check Sum=0000 0010+1000 1100+0000 0001+0101 1111=1110 1110 Check-sum=the last 8 bits of Sum=11101110

Signal & Timings:

The interval of whole process must be beyond 2 seconds.

To request data from DHT:
    Sent low pulse for > 1~10 ms (MILI SEC)
    Sent high pulse for > 20~40 us (Micros).
    When DHT detects the start signal, it will pull low the bus 80us as response signal, then the DHT pulls up 80us for preparation to send data.
    When DHT is sending data to MCU, every bit's transmission begin with low-voltage-level that last 50us, the following high-voltage-level signal's length decide the bit is "1" or "0". 0: 26~28 us 1: 70 us

*/
int32_t DHT22::waitOrTimeout(uint16_t microSeconds, int level) {
    int micros_ticks = 0;
    while(gpio_get_level(dht_gpio) == level) { 
        if(micros_ticks++ > microSeconds) 
            return -1;
        ets_delay_us(1);
    }
    return micros_ticks;
}

bool DHT22::checkCRC(uint8_t data[]) {
    if(data[4] == (data[0] + data[1] + data[2] + data[3]))
        return true;
    else
        return false;
}

void DHT22::sendStartSignal() {
    //set low for 1-10 ms
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    ets_delay_us(5 * 1000); //we'll pick middle
    gpio_set_level(dht_gpio, 1); //then high for 20-40 us
    ets_delay_us(30); //again we'll pick middle
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);
}

//DHT22 will pull signal low for 80 us then high for 80 us
ErrorType DHT22::checkResponse() {
	ErrorType et;
    /* Wait for next step ~80us*/
    if(waitOrTimeout(85, 0) == -1)
        et = ErrorType(ErrorType::TIMEOUT_ERROR);

    /* Wait for next step ~80us*/
    if(waitOrTimeout(85, 1) == -1)
        et = ErrorType(ErrorType::TIMEOUT_ERROR);

    return et;
}

DHT22::DHT22() : dht_gpio(NOPIN), LastReading(), LastReadTime(-2000000) {

}

ErrorType DHT22::init(gpio_num_t gpin) {
  ErrorType et;
	dht_gpio = gpin;
	LastReadTime = -2000000;
  return et;
}
	
void DHT22::shutdown() {

}

DHT22::~DHT22() {
  shutdown();
}

ErrorType DHT22::read(DHT22::Data &d) {
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
  	/* Read response */ //16 bits RH + 16 bits of Temp + 8 bit CRC
  	for(int i = 0; i < 40; i++) {
  	  /* Initial data */
  		if(waitOrTimeout(55, 0) != -1) { //every bit starts low for 50 us
        int32_t time = waitOrTimeout(70,1);
  			if(time > 28) { //means a 1
  				/* Bit received was a 1 */
  				data[i/8] |= (1 << (7-(i%8)));
  			} else if (time==-1) { // error
	  		  et = ErrorType::TIMEOUT_ERROR;
	  			break;
	  		} else {
          //just a 0
        }
	  	} else {
	  		et = ErrorType::TIMEOUT_ERROR;
	  		break;
	  	}
	  }
		if(et.ok()) {
			if(checkCRC(data)) {
        int32_t temp = int32_t(((data[2]&0x07)<<8) | data[3]); //remove the - sign if there and shift up 
				LastReading.temperature = float(temp)/float(10.0);
        if(data[2]&0x08) LastReading.temperature*=-1; //add sign back if need be
        int32_t hum = int32_t((data[0]<<8) | data[1]); 
				LastReading.humidity = float(hum)/float(10.0);
				d = LastReading;
			} else {
				et = ErrorType::DEVICE_CRC_ERROR;
			}
		}
  }
  return et;
}

