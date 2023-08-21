#pragma once

#include <cstdint>

namespace libesp {

enum DISPLAY_ROTATION {
   PORTAIT_TOP_LEFT = 0
   , LANDSCAPE_TOP_LEFT = 1
};

enum LIB_PIXEL_FORMAT {
   FORMAT_12_BIT
   , FORMAT_16_BIT
   , FORMAT_18_BIT 
};

class ColorPacker {
	public:
		ColorPacker();
		uint8_t *getPackedColorData();
		uint8_t getSize();
	public:
		static ColorPacker create (uint8_t pixelFormat, const RGBColor &c);
		static ColorPacker create2(uint8_t pixelFormat, const RGBColor &c);
	private:
		uint8_t Color[3];
		uint8_t SizeInBytes;
};

}
