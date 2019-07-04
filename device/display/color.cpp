/*
 * color.cpp
 *
 *  Created on: Jun 29, 2019
 *      Author: cmdc0de
 */

#include "color.h"

using namespace libesp;


const RGBColor RGBColor::BLACK(0, 0, 0);
const RGBColor RGBColor::RED(255, 0, 0);
const RGBColor RGBColor::GREEN(0, 255, 0);
const RGBColor RGBColor::BLUE(0, 0, 255);
const RGBColor RGBColor::WHITE(255, 255, 255);

bool RGBColor::operator==(const RGBColor &r) const {
	return (r.R == R && r.G == G && r.B == B);
}

bool RGBColor::operator!=(const RGBColor &r) const {
	return !((*this) == r);
}


///////////////////////////////////////////////////////////////////////////////////
PackedColor::PackedColor() :
		Color { 0 }, SizeInBytes(2) {
}

const uint8_t *PackedColor::getPackedColorData() const {
	return &Color[0];
}

uint8_t PackedColor::getSize() const {
	return SizeInBytes;
}

PackedColor PackedColor::create2(PIXEL_FORMAT pixelFormat, const RGBColor &c) {
	PackedColor pc;
	switch (pixelFormat) {
	case PIXEL_FORMAT_12_BIT:
		pc.SizeInBytes = 2;
		break;
	case PIXEL_FORMAT_16_BIT: {
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
	case PIXEL_FORMAT_18_BIT:
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

///virtual ctor for converting RGB color into packed color class for the driver chip
PackedColor PackedColor::create(PIXEL_FORMAT pixelFormat, const RGBColor &c) {
	PackedColor pc;
	switch (pixelFormat) {
	case PIXEL_FORMAT_12_BIT:
		pc.SizeInBytes = 2;
		break;
	case PIXEL_FORMAT_16_BIT: {
		uint16_t tmp;
		tmp = (c.getR() & 0b11111) << 11;
		tmp |= (c.getG() & 0b111111) << 5;
		tmp |= (c.getB() & 0b11111);
		pc.Color[0] = tmp >> 8;
		pc.Color[1] = tmp & 0xFF;
		pc.SizeInBytes = 2;
	}
		break;
	case PIXEL_FORMAT_18_BIT:
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
