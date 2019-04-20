
#ifndef LIBESP_DEVICE_APA102_H
#define LIBESP_DEVICE_APA102_H

/*
*	Brightness is a percentage
*/
class RGB {
public:
	static const RGB WHITE;
	static const RGB BLUE;
	static const RGB GREEN;
	static const RGB RED;
public:
	RGB() : B(0), G(0), R(0), Brightness(100) {}
	RGB(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) : B(b), G(g), R(r), Brightness(brightness) {}
	RGB(const RGB &r) : B(r.B), G(r.G), R(r.R), Brightness(r.Brightness) {}
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
public:
	APA102c(Wiring *spiI);
	void init(uint16_t nleds, RGB *ledBuf);
	void send();
private:
	Wiring *SPIInterface;
	uint16_t BufferSize;
	char *LedBuffer1;
};

#endif
