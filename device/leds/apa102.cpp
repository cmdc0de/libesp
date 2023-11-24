#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include <system.h>
#include <spidevice.h>
#include <esp_log.h>
#include "apa102.h"

using namespace libesp;

const RGBB RGBB::WHITE(255,255,255,100);
const RGBB RGBB::BLUE(0,0,255,100);
const RGBB RGBB::GREEN(0,255,0,100);
const RGBB RGBB::RED(255,0,0,100);

const char *APA102c::LOG = "APA102c";


ErrorType APA102c::initAPA102c(gpio_num_t mosi, gpio_num_t clk
		, spi_host_device_t spiNum, int channel) {
	ErrorType et;
	spi_bus_config_t buscfg;
   memset(&buscfg,0, sizeof(buscfg));
   buscfg.miso_io_num=-1;
   buscfg.mosi_io_num=mosi;
   buscfg.sclk_io_num=clk;
   buscfg.quadwp_io_num=-1;
   buscfg.quadhd_io_num=-1;
   buscfg.max_transfer_sz=0;
   buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
   buscfg.intr_flags = 0;

	et = libesp::SPIBus::initializeBus(spiNum,buscfg,channel);
	if(!et.ok()) {
		ESP_LOGE(LOG, "Error initializing SPI Bus: %s", et.toString());
	}

	return et;
}

APA102c::APA102c() : SPIInterface(0), BufferSize(0), LedBuffer1(0) {}

ErrorType APA102c::initDevice(libesp::SPIBus *bus) {
  ErrorType et;

	spi_device_interface_config_t devcfg;
	memset(&devcfg,0,sizeof(devcfg));
	devcfg.clock_speed_hz=1*1000*1000;
	devcfg.mode=0;          //SPI mode 0
	devcfg.spics_io_num=-1; //CS pin
	devcfg.queue_size=3; //We want to be able to queue 3 transactions at a time
	devcfg.duty_cycle_pos = 0;
	devcfg.cs_ena_pretrans = 0;
	devcfg.cs_ena_posttrans = 0; 
	devcfg.input_delay_ns = 0;
	devcfg.flags = 0;
	devcfg.pre_cb = nullptr;
	devcfg.post_cb = nullptr;

	SPIInterface = bus->createMasterDevice(devcfg);
  return et;
}

void APA102c::init(uint16_t nleds, RGBB *ledBuf) {
	delete [] LedBuffer1;
	BufferSize = (nleds*4)+8;
	LedBuffer1 = new char [BufferSize];
	int bufOff = 0;
	LedBuffer1[bufOff]   = 0x0;
	LedBuffer1[++bufOff] = 0x0;
	LedBuffer1[++bufOff] = 0x0;
	LedBuffer1[++bufOff] = 0x0;
	for(int l=0;l<nleds;++l) {
		uint8_t bright = ledBuf[l].getBrightness();
		bright = (uint8_t)(((float)bright/100.0f)*MAX_BRIGHTNESS);
		LedBuffer1[++bufOff] = BRIGHTNESS_START_BITS|bright;
		LedBuffer1[++bufOff] = ledBuf[l].getBlue();
		LedBuffer1[++bufOff] = ledBuf[l].getGreen();
		LedBuffer1[++bufOff] = ledBuf[l].getRed();
	}
	LedBuffer1[++bufOff] = 0xFF;
	LedBuffer1[++bufOff] = 0xFF;
	LedBuffer1[++bufOff] = 0xFF;
	LedBuffer1[++bufOff] = 0xFF;
}


void APA102c::send() {
	if(BufferSize>0) {
		//ESP_LOGI(APA102c::LOG,"sending %d leds r[0]:%d, g[0]:%d, b[0]:%d, B[0]:%d\n", (BufferSize/4)-8,
	//		LedBuffer1[3], LedBuffer1[2], LedBuffer1[1], LedBuffer1[0]);
		//ESP_LOG_BUFFER_HEX(APA102c::LOG, LedBuffer1, BufferSize);
		SPIInterface->send((uint8_t*)LedBuffer1,BufferSize);
	}
}



