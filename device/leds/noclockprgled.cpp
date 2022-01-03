
#include "noclockprgled.h"
#include <cstring>

using namespace libesp;

typedef uint8_t fract8;

///  scale one byte by a second one, which is treated as
/////  the numerator of a fraction whose denominator is 256
/////  In other words, it computes i * (scale / 256)
/////  4 clocks AVR with MUL, 2 clocks ARM
uint8_t scale8(uint8_t i, fract8 scale) {
	return (((uint16_t) i) * (1 + (uint16_t) (scale))) >> 8;
}

///  The "video" version of scale8 guarantees that the output will
///  be only be zero if one or both of the inputs are zero.  If both
///  inputs are non-zero, the output is guaranteed to be non-zero.
///  This makes for better 'video'/LED dimming, at the cost of
///  several additional cycles.
uint8_t scale8_video(uint8_t i, fract8 scale) {
	return (((int) i * (int) scale) >> 8) + ((i && scale) ? 1 : 0);
}

/////

static void IRAM_ATTR _rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size, size_t wanted_num, size_t *translated_size, size_t *item_num, const rmt_item32_t *bit0, const rmt_item32_t *bit1) {
	if (!src || !dest) {
		*translated_size = 0;
		*item_num = 0;
		return;
	}

	size_t size = 0;
	size_t num = 0;
	uint8_t *psrc = (uint8_t *)src;
	rmt_item32_t *pdest = dest;
	NoClkLedStrip *strip;
	esp_err_t r = rmt_translator_get_context(item_num, (void **)&strip);
	uint8_t brightness = r == ESP_OK ? strip->getBrightness() : 255;


	while (size < src_size && num < wanted_num) {
		uint8_t b = brightness != 255 ? scale8_video(*psrc, brightness) : *psrc;
		for (int i = 0; i < 8; i++) {
			// MSB first
			pdest->val = b & (1 << (7 - i)) ? bit1->val : bit0->val;
			num++;
			pdest++;
		}
		size++;
		psrc++;
	}
	*translated_size = size;
	*item_num = num;
}

//LEDBase Type

LEDBaseType::LEDBaseType(uint32_t t0hns, uint32_t t0lns, uint32_t t1hns, uint32_t t1lns ) 
	: T0H_NS(t0hns), T0L_NS(t0lns), T1H_NS(t1hns), T1L_NS(t1lns) {

	float ratio = (float)(APB_CLK_FREQ / LED_STRIP_RMT_CLK_DIV) / 1e09;
	Bit0.duration0 = ratio * T0H_NS;
	Bit0.level0 = 1;
	Bit0.duration1 = ratio * T0L_NS;
	Bit0.level1 = 0;
	Bit1.duration0 = ratio * T1H_NS;
	Bit1.level0 = 1;
	Bit1.duration1 = ratio * T1L_NS;
	Bit1.level1 = 0;	
}


//APA106
//

APA106 &APA106::get() {
	static APA106 APA106Self;
	return APA106Self;
}

void IRAM_ATTR APA106::rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,size_t wanted_num, size_t *translated_size, size_t *item_num) {
	_rmt_adapter(src,dest,src_size,wanted_num, translated_size, item_num, APA106::get().getBit0(), APA106::get().getBit1());
}

//noclkled
//

const char *NoClkLedStrip::LOGTAG = "NoClkLedStrip";

NoClkLedStrip::NoClkLedStrip(const LEDBaseType &lbt, uint8_t b, size_t len, uint8_t *data) : LedType(lbt), Config(), Brightness(b), StripLen(len), LedStrip(data) {
	memset(&Config,0,sizeof(rmt_config_t));
}

NoClkLedStrip NoClkLedStrip::create(const LEDBaseType &ledBase, uint8_t brightness, size_t len) {
	uint8_t *leds = new uint8_t[len*ledBase.getBytesPerLED()];
	NoClkLedStrip strip(ledBase, brightness, len, leds);
	//strip.init(dataPin,channel);
	return strip;
}

ErrorType NoClkLedStrip::init(gpio_num_t dataPin, rmt_channel_t channel) {
	ErrorType retVal;

	static const uint32_t LED_STRIP_RMT_CLK_DIV=2;
	Config.rmt_mode = RMT_MODE_TX;
	Config.channel = channel;
	Config.gpio_num = dataPin;
	Config.clk_div = LED_STRIP_RMT_CLK_DIV;
	Config.mem_block_num = 1;
	Config.tx_config.carrier_freq_hz = 38000;
	Config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
	Config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
	Config.tx_config.carrier_duty_percent = 33;
	Config.tx_config.carrier_en = false;
	Config.tx_config.loop_en = false;
	Config.tx_config.idle_output_en = true;

	retVal = rmt_config(&Config);
	if(retVal.ok()) {
		retVal = rmt_driver_install(Config.channel,0,0);
		if(retVal.ok()) {
			retVal = rmt_translator_init(Config.channel,LedType.getTranslatorFun());
			if(retVal.ok()) {
				retVal = rmt_translator_set_context(Config.channel,this);
				if(!retVal.ok()) {
					ESP_LOGE(LOGTAG,"rmt_Translator set context: %s", retVal.toString());
				}
			}	else {
				ESP_LOGE(LOGTAG,"rmt_translator_init %s", retVal.toString());
			} 
		} else {
			ESP_LOGE(LOGTAG,"rmt_driver_install %s",retVal.toString());
		}
	} else {
		ESP_LOGE(LOGTAG,"rmt_config %s",retVal.toString());
	}
	return retVal;
}

void NoClkLedStrip::setColor(size_t index, const RGB &ledColor) {
	uint32_t start = index*LedType.getBytesPerLED();
	LedStrip[start] = ledColor.getRed();
	LedStrip[start + 1] = ledColor.getGreen();
	LedStrip[start + 2] = ledColor.getBlue();
}

void NoClkLedStrip::fillColor(size_t startIndex, size_t lastIndex, const RGB &ledColor) {
	for(size_t i = startIndex;i<lastIndex;++i) {
		uint32_t start = i*LedType.getBytesPerLED();
		LedStrip[start] = ledColor.getRed();
		LedStrip[start + 1] = ledColor.getGreen();
		LedStrip[start + 2] = ledColor.getBlue();
	}
}


void NoClkLedStrip::fillColor(const RGB &color) {
	fillColor(0,StripLen,color);	
}


RGB NoClkLedStrip::getColor(size_t index) {
	uint32_t start = index*LedType.getBytesPerLED();
	return RGB(LedStrip[start],LedStrip[start+1],LedStrip[start+2]);	
}

void NoClkLedStrip::release() {
	delete [] LedStrip;
	LedStrip = nullptr;
}

ErrorType NoClkLedStrip::send() {
	return send(DEFAULT_SEND_WAIT);
}

ErrorType NoClkLedStrip::send(TickType_t  waitTime) {
	ErrorType et;
	et = rmt_wait_tx_done(Config.channel,waitTime);
	if(et.ok()) {
		et = rmt_write_sample(Config.channel,LedStrip,StripLen*LedType.getBytesPerLED(),false);
		if(!et.ok()) {
			ESP_LOGE(LOGTAG,"rmt write_sample %s", et.toString());
		}
	} else {
		ESP_LOGE(LOGTAG,"can't send: %s", et.toString());
	}
	return et;
}


ErrorType NoClkLedStrip::sendBlocking() {
	ErrorType et;
	et = canSend();
	if(et.ok()) {
		et = rmt_write_sample(Config.channel,LedStrip,StripLen*LedType.getBytesPerLED(),true);
		if(!et.ok()) {
			ESP_LOGE(LOGTAG,"rmt write_sample %s", et.toString());
		}
	} else {
		ESP_LOGE(LOGTAG,"can't send: %s", et.toString());
	}
	return et;
}



bool NoClkLedStrip::canSend() {
	return rmt_wait_tx_done(Config.channel, 0) == ESP_ERR_TIMEOUT;
}

NoClkLedStrip::~NoClkLedStrip() {
	release();
}



