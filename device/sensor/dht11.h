#ifndef LIBESP_DHT11_H_
#define LIBESPDHT11_H_

#include "driver/gpio.h"

namespace libesp {

class DHT11 {
public:
	struct Data {
		Data() : temperature(0), humidity(0) {}
	    int temperature;
	    int humidity;
	};
public:
	DHT11();
	ErrorType init(gpio_num_t p);
	ErrorType read(DHT11::Data &d);
	void shutdown();
	~DHT11();
protected:
	void sendStartSignal();
	void checkResponse();
	int32_t waitOrTimeout(uint16_t microSeconds, int level);
	bool checkCRC(uint8_t data[]);
private:
	gpio_num_t dht_gpio;
	Data LastReading;
	int64_t LastReadTime;

};
void DHT11_init(gpio_num_t);

struct dht11_reading DHT11_read();

}

#endif
