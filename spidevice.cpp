#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error_type.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include <libesp/system.h>
#include <libesp/i2c.hpp>
#include <esp_log.h>
#include "wiring.h"

Wiring::Wiring() {
}

bool Wiring::init() {
	return onInit();
}
	
ErrorType Wiring::shutdown() {
	return onShutdown();
}

Wiring::~Wiring() {

}


ESP32SPIWiring ESP32SPIWiring::create(spi_host_device_t shd, int miso, int mosi, int clk, int cs, int tb, int dma) {
	ESP32SPIWiring esp32(shd,miso,mosi,clk,cs, tb,dma);
	return esp32;
}
	
ErrorType ESP32SPIWiring::sendAndReceive(uint8_t out, uint8_t &in) {
	spi_transaction_t t;
    	memset(&t, 0, sizeof(t));   //Zero out the transaction
    	t.length=8;                 //Len is in bytes, transaction length is in bits.
	t.tx_data[0]=out;             //Data
	t.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
	t.user=(void*)1;              
    	esp_err_t r = spi_device_transmit(spi, &t);  //Transmit!
	in = t.rx_data[0];
	return r;
}

ErrorType ESP32SPIWiring::send(uint8_t *p, uint16_t len) {
    	if (len==0) return ESP_OK;       //no need to send anything
    	spi_transaction_t t;
    	memset(&t, 0, sizeof(t));   //Zero out the transaction
    	t.length=len*8;                 //Len is in bytes, transaction length is in bits.
	t.tx_buffer=p;             //Data
	t.user=(void*)2;              
    	esp_err_t r = spi_device_transmit(spi, &t);  //Transmit!
	return r;
}

ESP32SPIWiring::~ESP32SPIWiring() {
	shutdown();
}

ErrorType ESP32SPIWiring::onShutdown() {
	return spi_bus_free(SpiHD);
}

ESP32SPIWiring ESP32SPIWiring::create(spi_host_device_t shd,spi_bus_config_t &buscfg,spi_device_interface_config_t &devcfg, int dmaChannel)
	: BusCfg(buscfg), DevCfg(devcfg), DMAChannel(dmaChannel) {
	
}

bool ESP32SPIWiring::onInit() {
	esp_err_t ret;

	//Initialize the SPI bus
	ret=spi_bus_initialize(SpiHD, &Buscfg, DMAChannel);
	ESP_ERROR_CHECK(ret);

	//Attach the LED to the SPI bus
	ret=spi_bus_add_device(SpiHD, &DevCfg, &spi);
	ESP_ERROR_CHECK(ret);
	return ESP_OK==ret;
}

