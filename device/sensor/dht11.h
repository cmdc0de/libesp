#ifndef LIBESP_DHT11_H_
#define LIBESPDHT11_H_

#include "driver/gpio.h"

namespace libesp {

enum dht11_status {
    DHT11_CRC_ERROR = -2,
    DHT11_TIMEOUT_ERROR,
    DHT11_OK
};

struct dht11_reading {
    int status;
    int temperature;
    int humidity;
};

void DHT11_init(gpio_num_t);

struct dht11_reading DHT11_read();

}

#endif
