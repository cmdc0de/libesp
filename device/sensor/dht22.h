#ifndef LIBESP_DHT22_H_
#define LIBESPDHT22_H_

#pragma once

#include "../../error_type.h"
#include "driver/gpio.h"

namespace libesp {

  /*
   * 
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

class DHT22 {
public:
	struct Data {
		Data() : temperature(0.0f), humidity(0.0f) {}
	    float temperature;
	    float humidity;
	};
public:
	DHT22();
	ErrorType init(gpio_num_t p);
	ErrorType read(DHT22::Data &d);
	void shutdown();
	~DHT22();
protected:
	void sendStartSignal();
  ErrorType checkResponse();
	int32_t waitOrTimeout(uint16_t microSeconds, int level);
	bool checkCRC(uint8_t data[]);
private:
	gpio_num_t dht_gpio;
	Data LastReading;
	int64_t LastReadTime;

};

}
#endif
