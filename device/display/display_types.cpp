#include "display_types.h"

using namespace libesp;

ColorPacker::ColorPacker() : Color(), SizeInBytes(0) {

}

const uint8_t *ColorPacker::getPackedColorData() const {
   return &Color[0];
}

uint8_t ColorPacker::getSize() const {
   return SizeInBytes;
}

ColorPacker ColorPacker::create (LIB_PIXEL_FORMAT pixelFormat, const RGBColor &c) {
	ColorPacker pc;
	switch (pixelFormat) {
      case libesp::LIB_PIXEL_FORMAT::FORMAT_12_BIT:
		   pc.SizeInBytes = 2;
		break;
      case libesp::LIB_PIXEL_FORMAT::FORMAT_16_BIT: {
		   uint16_t tmp;
   		tmp = (c.getR() & 0b11111) << 11;
   		tmp |= (c.getG() & 0b111111) << 5;
   		tmp |= (c.getB() & 0b11111);
   		pc.Color[0] = tmp >> 8;
   		pc.Color[1] = tmp & 0xFF;
   		pc.SizeInBytes = 2;
	   }
		break;
      case libesp::LIB_PIXEL_FORMAT::FORMAT_18_BIT:
		   pc.Color[0] = c.getR() << 2;
		   pc.Color[1] = c.getG() << 2;
		   pc.Color[2] = c.getB() << 2;
		   pc.SizeInBytes = 3;
		break;
	default:
		//assert(false);
		break;
	}
   return pc;
}

ColorPacker ColorPacker::create2(LIB_PIXEL_FORMAT pixelFormat, const RGBColor &c) {
	ColorPacker pc;
	switch (pixelFormat) {
      case libesp::LIB_PIXEL_FORMAT::FORMAT_12_BIT:
		pc.SizeInBytes = 2;
		break;
      case libesp::LIB_PIXEL_FORMAT::FORMAT_16_BIT: {
		uint32_t rc = c.getR()/8; //keep it in 5 bits
		uint32_t gc = c.getG()/4; //keep it in 6 bits
		uint32_t bc = c.getB()/8; //keep it in 5 bits
		uint16_t tmp;
		tmp = (rc & 0b11111) << 11;
		tmp |= (gc & 0b111111) << 5;
		tmp |= (bc & 0b11111);
		pc.Color[0] = tmp >> 8;
		pc.Color[1] = tmp & 0xFF;
		pc.SizeInBytes = 2;
	}
		break;
      case libesp::LIB_PIXEL_FORMAT::FORMAT_18_BIT:
		pc.Color[0] = c.getR() << 2;
		pc.Color[1] = c.getG() << 2;
		pc.Color[2] = c.getB() << 2;
		pc.SizeInBytes = 3;
		break;
	default:
		//assert(false);
		break;
	}
	return pc;
}
