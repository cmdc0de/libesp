#pragma once

#include "led_color.h"
#include <driver/rmt.h>
#include <driver/gpio.h>
#include "../../error_type.h"

namespace libesp {


class LEDBaseType {
public:
	static const uint32_t LED_STRIP_RMT_CLK_DIV=2;
public:
	LEDBaseType(uint32_t t0hns, uint32_t t0lns, uint32_t t1hns, uint32_t t1lns);
	rmt_item32_t *getBit0() {return &Bit0;}
	rmt_item32_t *getBit1() {return &Bit1;}
	virtual uint8_t getBytesPerLED() const {return 3;}
	virtual sample_to_rmt_t getTranslatorFun() const = 0;
private:
	const uint32_t T0H_NS;
	const uint32_t T0L_NS;
	const uint32_t T1H_NS;
	const uint32_t T1L_NS;
private:
	rmt_item32_t Bit0;
	rmt_item32_t Bit1;

};

/*
class WS2812 : public LEDBaseType {
public:
	WS2812() : LEDBaseType(400,1000,1000,400) {}
};

class SK6812 {
public:
	SK6812() : LEDBaseType(300,900,600,600) {}

};
*/

class APA106 : public LEDBaseType {
public:
	static void IRAM_ATTR rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,size_t wanted_num, size_t *translated_size, size_t *item_num);
	virtual sample_to_rmt_t getTranslatorFun() const override {
		return rmt_adapter;
	}
	static APA106 &get();
protected:
	APA106() : LEDBaseType(350,1360,1360,350) {}
};

class APA104 : public APA106 {
public:
	APA104() : APA106() {}
};


/*
 * Programmable LEDs that have a fixed clock APA106, 104's etc.
 * 
 * here we'll use the remote function of the ESP32...this grows the amount of memory needed but we'll try this for now.
 *
*/
class NoClkLedStrip {
public:
	static const TickType_t DEFAULT_SEND_WAIT = pdMS_TO_TICKS(1000); 
	static const char *LOGTAG;
	static NoClkLedStrip create(const LEDBaseType &ledBase, uint8_t brightness, size_t len);
public:
	ErrorType init(gpio_num_t dataPin, rmt_channel_t channel);
	void setColor(size_t index, const RGB &ledColor);
	void fillColor(size_t startIndex, size_t lastIndex, const RGB &ledColor);
	void fillColor(const RGB &color);
	RGB getColor(size_t index);
	void release();
	ErrorType send();
	ErrorType send(TickType_t ticksToWait);
	ErrorType sendBlocking();
	bool canSend();
	~NoClkLedStrip();
public:
	void setBrightness(uint8_t b) {Brightness = b;}
	uint8_t getBrightness() {return Brightness;}
	const LEDBaseType &getLedType() const { return LedType;}
protected:
	NoClkLedStrip(const LEDBaseType &lbt, uint8_t b, size_t len, uint8_t *data);

private:
	const LEDBaseType &LedType;
	rmt_config_t Config;
	uint8_t Brightness;
	size_t StripLen;
	uint8_t *LedStrip;
};


}
