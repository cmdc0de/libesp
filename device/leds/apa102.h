
#ifndef LIBESP_DEVICE_APA102_H
#define LIBESP_DEVICE_APA102_H

namespace libesp {

class SPIDevice;

/*
*	Red
*	Green
*	Blue
*	Brightness - Brightness is a percentage
*/
class RGBB {
public:
	static const RGBB WHITE;
	static const RGBB BLUE;
	static const RGBB GREEN;
	static const RGBB RED;
public:
	RGBB() : B(0), G(0), R(0), Brightness(100) {}
	RGBB(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) : B(b), G(g), R(r), Brightness(brightness) {}
	RGBB(const RGBB &r) : B(r.B), G(r.G), R(r.R), Brightness(r.Brightness) {}
	uint8_t getBlue()  {return B;}
	uint8_t getRed()   {return R;}
	uint8_t getGreen() {return G;}
	uint8_t getBrightness() {return Brightness;}
	void setBlue(uint8_t v) 	{B=v;}
	void setRed(uint8_t v)  	{R=v;}
	void setGreen(uint8_t v) 	{G=v;}
	void setBrightness(uint8_t v)	{Brightness=((v>100)?100:v);}
private:
	uint8_t B;
	uint8_t G;
	uint8_t R;
	uint8_t Brightness;
} __attribute__((packed));;


class APA102c {
public:
	//0xE0 because high 3 bits are always on
	static const uint8_t BRIGHTNESS_START_BITS = 0xE0;
	static const uint8_t MAX_BRIGHTNESS			 = 31;
	static const char *LOG;
  static ErrorType initAPA102c(gpio_num_t mosi, gpio_num_t clk, spi_host_device_t spiNum, int channel);
public:
	APA102c();
  ErrorType initDevice(libesp::SPIBus *bus);
	void init(uint16_t nleds, RGBB *ledBuf);
	void send();
private:
	SPIDevice *SPIInterface;
	uint16_t BufferSize;
	char *LedBuffer1;
};


}

#endif
