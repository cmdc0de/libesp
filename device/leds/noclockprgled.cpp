
#include "noclockprgled.h"
#include "led_strip_encoder.h"
#include <cstring>

using namespace libesp;

typedef uint8_t fract8;

static const char *LOGTAG = "noclkprgled";
static const uint32_t LED_STRIP_RMT_CLK_DIV=2;
static const uint32_t  RMT_LED_STRIP_RESOLUTION_HZ=10000000; // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

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

static void IRAM_ATTR _rmt_adapter(const void *src, rmt_symbol_word_t *dest, size_t src_size, size_t wanted_num, size_t *translated_size, size_t *item_num, const rmt_symbol_word_t *bit0, const rmt_symbol_word_t *bit1) {
	if (!src || !dest) {
		*translated_size = 0;
		*item_num = 0;
		return;
	}

	size_t size = 0;
	size_t num = 0;
	uint8_t *psrc = (uint8_t *)src;
	rmt_symbol_word_t *pdest = dest;
	NoClkLedStrip *strip;
	ErrorType et;
	//ErrorType et = rmt_translator_get_context(item_num, (void **)&strip);
	uint8_t brightness = 255;
	if(et.ok()) {
		brightness = strip->getBrightness();
	} else {
		ESP_LOGE(LOGTAG, "Error getting context: %s", et.toString());
	}

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

LEDBaseType::LEDBaseType(uint32_t t0hns, uint32_t t0lns, uint32_t t1hns, uint32_t t1lns, LEDBaseType::ORDER o ) 
	: T0H_NS(t0hns), T0L_NS(t0lns), T1H_NS(t1hns), T1L_NS(t1lns), Order(o) {

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

void IRAM_ATTR APA106::rmt_adapter(const void *src, rmt_symbol_word_t *dest, size_t src_size,size_t wanted_num, size_t *translated_size, size_t *item_num) {
	_rmt_adapter(src,dest,src_size,wanted_num, translated_size, item_num, APA106::get().getBit0(), APA106::get().getBit1());
}

//APA104

APA104 &APA104::get() {
    static APA104 APA104Self;
    return APA104Self;
}

void IRAM_ATTR APA104::rmt_adapter(const void *src, rmt_symbol_word_t *dest, size_t src_size,size_t wanted_num, size_t *translated_size, size_t *item_num) {
	_rmt_adapter(src,dest,src_size,wanted_num, translated_size, item_num, APA104::get().getBit0(), APA104::get().getBit1());
}

//noclkled
//

const char *NoClkLedStrip::LOGTAG = "NoClkLedStrip";

NoClkLedStrip::NoClkLedStrip(const LEDBaseType &lbt, uint8_t b, size_t len, uint8_t *data) 
	: LedType(lbt), ChannelHandle(), ConfigChannel(), EncoderHandle(), Brightness(b), StripLen(len), LedStrip(data) {
	memset(&ConfigChannel,0,sizeof(rmt_tx_channel_config_t));
	memset(&ChannelHandle,0,sizeof(rmt_channel_handle_t));
	memset(&EncoderHandle,0,sizeof(EncoderHandle));
}


NoClkLedStrip NoClkLedStrip::create(const LEDBaseType &ledBase, uint8_t brightness, size_t len) {
	uint8_t *leds = new uint8_t[len*ledBase.getBytesPerLED()];
	NoClkLedStrip strip(ledBase, brightness, len, leds);
	return strip;
}

ErrorType NoClkLedStrip::init(gpio_num_t dataPin) {
	ErrorType retVal;
	
	ConfigChannel.gpio_num = dataPin;
	ConfigChannel.clk_src = RMT_CLK_SRC_DEFAULT;
	ConfigChannel.resolution_hz = 38000;
	ConfigChannel.mem_block_symbols = StripLen;
	ConfigChannel.trans_queue_depth =	1; // not sure what to set this to
	//ConfigChannel.intr_priority = 0; // not sure what to set this to either - doc points to 0 
	ConfigChannel.flags.invert_out = 0;
	ConfigChannel.flags.with_dma = 1; // should probably be a parameter!
	//ConfigChannel.io_loop_back = 0;
	//ConfigChannel.io_od_back = 0;
	//ConfigChannel.allow_pd = 0; // should b e a parameter!
	//ConfigChannel.init_level = 0; // should be a parameter

	retVal = rmt_new_tx_channel(&ConfigChannel, &ChannelHandle);
	if(retVal.ok()) {
		led_strip_encoder_config_t encoder_config = {0};
		encoder_config.resolution = RMT_LED_STRIP_RESOLUTION_HZ;
		retVal = rmt_new_led_strip_encoder(&encoder_config, &EncoderHandle);
		if(retVal.ok()) {
			retVal = rmt_enable(ChannelHandle);
		} else {
			ESP_LOGE(LOGTAG,"failed new led_strip_encode: %s", retVal.toString());
		}
	} else {
		ESP_LOGE(LOGTAG,"failed new tx channel: %s", retVal.toString());
	}
	return retVal;
}

void NoClkLedStrip::setColor(size_t index, const RGB &ledColor) {
	uint32_t start = index*LedType.getBytesPerLED();
  switch(LedType.getOrder()) {
		case LEDBaseType::ORDER::RGB:
      LedStrip[start] = ledColor.getRed();
		  LedStrip[start + 1] = ledColor.getGreen();
		  LedStrip[start + 2] = ledColor.getBlue();
      break;
    case LEDBaseType::ORDER::GRB:
      LedStrip[start] = ledColor.getGreen();
		  LedStrip[start + 1] = ledColor.getRed();
		  LedStrip[start + 2] = ledColor.getBlue();
      break;
  }
}

void NoClkLedStrip::fillColor(size_t startIndex, size_t lastIndex, const RGB &ledColor) {
	for(size_t i = startIndex;i<lastIndex;++i) {
		uint32_t start = i*LedType.getBytesPerLED();
    switch(LedType.getOrder()) {
		  case LEDBaseType::ORDER::RGB:
        LedStrip[start] = ledColor.getRed();
		    LedStrip[start + 1] = ledColor.getGreen();
		    LedStrip[start + 2] = ledColor.getBlue();
        break;
      case LEDBaseType::ORDER::GRB:
        LedStrip[start] = ledColor.getGreen();
		    LedStrip[start + 1] = ledColor.getRed(); 
		    LedStrip[start + 2] = ledColor.getBlue();
        break;
    }
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
	et = rmt_tx_wait_all_done(ChannelHandle,waitTime);
	if(et.ok()) {
		rmt_transmit_config_t tx_config;
		memset(&tx_config,0,sizeof(tx_config));
		tx_config.loop_count = 0;
		et = rmt_transmit(ChannelHandle, EncoderHandle, &LedStrip[0], StripLen*LedType.getBytesPerLED(), &tx_config);
		if(!et.ok()) {
			ESP_LOGE(LOGTAG,"rmt transmit %s", et.toString());
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
		rmt_transmit_config_t tx_config;
		memset(&tx_config,0,sizeof(tx_config));
		tx_config.loop_count = 0;
		et = rmt_transmit(ChannelHandle, EncoderHandle, &LedStrip[0], StripLen*LedType.getBytesPerLED(), &tx_config);
		if(!et.ok()) {
			ESP_LOGE(LOGTAG,"rmt write_sample %s", et.toString());
		}
	} else {
		ESP_LOGE(LOGTAG,"can't send: %s", et.toString());
	}
	return et;
}



bool NoClkLedStrip::canSend() {
	return rmt_tx_wait_all_done(ChannelHandle, 0) == ESP_ERR_TIMEOUT;
}

NoClkLedStrip::~NoClkLedStrip() {
	release();
}



