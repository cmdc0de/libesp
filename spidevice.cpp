//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include "spidevice.h"
#include "error_type.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include <libesp/system.h>
#include <esp_log.h>

using namespace libesp;

//base spi device

SPIDevice::SPIDevice(SPIBus *b, const spi_device_handle_t &sdh, const spi_device_interface_config_t &devcfg) : MyBus(b), SPIHandle(sdh), DevCfg(devcfg) {

}

bool SPIDevice::init() {
	return onInit();
}
	
ErrorType SPIDevice::shutdown() {
	return onShutdown();
}

ErrorType SPIDevice::sendAndReceive(uint8_t out, uint8_t &in) {
	return onSendAndReceive(out,in);
}

ErrorType SPIDevice::send(uint8_t *p, uint16_t len) {
	return onSend(p,len);
}

SPIDevice::~SPIDevice() {

}


////////////////////////////////
//master

ErrorType SPIMaster::onSendAndReceive(uint8_t out, uint8_t &in) {
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));   //Zero out the transaction
	t.length=8;                 //Len is in bytes, transaction length is in bits.
	t.tx_data[0]=out;             //Data
	t.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
	t.user=(void*)1;              
	esp_err_t r = spi_device_transmit(SPIHandle, &t);  //Transmit!
	in = t.rx_data[0];
	return r;
}

ErrorType SPIMaster::onSend(uint8_t *p, uint16_t len) {
	if (len==0) return ESP_OK;       //no need to send anything
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));   //Zero out the transaction
	t.length=len*8;                 //Len is in bytes, transaction length is in bits.
	t.tx_buffer=p;             //Data
	//t.user=(void*)2;              
	esp_err_t r = spi_device_transmit(SPIHandle, &t);  //Transmit!
	return r;
}

SPIMaster::~SPIMaster() {
	shutdown();
}

ErrorType SPIMaster::onShutdown() {
	return getBus()->removeDevice(this);
}

SPIMaster::SPIMaster(SPIBus* s, const spi_device_handle_t &sdh, const spi_device_interface_config_t &devcfg)
	: SPIDevice(s,sdh,devcfg) {
	
}

bool SPIMaster::onInit() {
	return true;
}

