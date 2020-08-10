#ifndef _LIBESP_LEDCOLOR_H
#define _LIBESP_LEDCOLOR_H

#include <stdint.h>

namespace libesp {

class RGB {
public:
	static const RGB WHITE;
	static const RGB BLUE;
	static const RGB GREEN;
	static const RGB RED;
public:
	RGB() : B(0), G(0), R(0) {}
	RGB(uint8_t r, uint8_t g, uint8_t b) : B(b), G(g), R(r) {}
	RGB(const RGB &r) : B(r.B), G(r.G), R(r.R) {}
	uint8_t getBlue()  {return B;}
	uint8_t getRed()   {return R;}
	uint8_t getGreen() {return G;}
	void setBlue(uint8_t v) 	{B=v;}
	void setRed(uint8_t v)  	{R=v;}
	void setGreen(uint8_t v) 	{G=v;}
private:
	uint8_t B;
	uint8_t G;
	uint8_t R;
} __attribute__((packed));;

}

#endif
