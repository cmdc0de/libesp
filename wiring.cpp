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

ESP32SPIWiring::ESP32SPIWiring(spi_host_device_t spihd, int miso, int mosi, int clk, int cs, int bufSize, int dmachannel) 
	: SpiHD(spihd), PinMiso(miso), PinMosi(mosi), PinCLK(clk), PinCS(cs), TransferBufferSize(bufSize), DMAChannel(dmachannel) { 
	
}

bool ESP32SPIWiring::onInit() {
	esp_err_t ret;

	spi_bus_config_t buscfg;
	buscfg.miso_io_num=PinMiso;
	buscfg.mosi_io_num=PinMosi;
	buscfg.sclk_io_num=PinCLK;
	buscfg.quadwp_io_num=-1;
	buscfg.quadhd_io_num=-1;
	buscfg.max_transfer_sz=TransferBufferSize;
	buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
	buscfg.intr_flags = 0;

	//Initialize the SPI bus
	ret=spi_bus_initialize(SpiHD, &buscfg, DMAChannel);
	ESP_ERROR_CHECK(ret);

	spi_device_interface_config_t devcfg;
	devcfg.clock_speed_hz=1*1000*1000;         //Clock out at 1 MHz
	devcfg.mode=0;                             //SPI mode 0
	devcfg.spics_io_num=PinCS;               	//CS pin
	devcfg.queue_size=3;                       //We want to be able to queue 3 transactions at a time
	devcfg.duty_cycle_pos = 0;
	devcfg.cs_ena_pretrans = 0;
	devcfg.cs_ena_posttrans = 0; 
	devcfg.input_delay_ns = 0;
	devcfg.flags = 0;
	devcfg.pre_cb = nullptr;
	devcfg.post_cb = nullptr;

	//Attach the LED to the SPI bus
	ret=spi_bus_add_device(SpiHD, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);
	return ESP_OK==ret;
}

