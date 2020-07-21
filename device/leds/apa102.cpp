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

const RGB RGB::WHITE(255,255,255,100);
const RGB RGB::BLUE(0,0,255,100);
const RGB RGB::GREEN(0,255,0,100);
const RGB RGB::RED(255,0,0,100);

const char *APA102c::LOG = "APA102c";

APA102c::APA102c(SPIDevice *spiI) : SPIInterface(spiI), BufferSize(0), LedBuffer1(0) {}
	
void APA102c::init(uint16_t nleds, RGB *ledBuf) {
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
		ESP_LOGI(APA102c::LOG,"sending %d leds r[0]:%d, g[0]:%d, b[0]:%d, B[0]:%d\n", (BufferSize/4)-8,
			LedBuffer1[3], LedBuffer1[2], LedBuffer1[1], LedBuffer1[0]);
		ESP_LOG_BUFFER_HEX(APA102c::LOG, LedBuffer1, BufferSize);
		SPIInterface->send((uint8_t*)LedBuffer1,BufferSize);
	}
}



