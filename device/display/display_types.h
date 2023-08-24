#pragma once

#include <cstdint>

namespace libesp {

enum DISPLAY_ROTATION {
     PORTAIT_TOP_LEFT = 0
   , PORTAIT_TOP_RIGHT = 1
   , PORTAIT_BOTTOM_LEFT = 2
   , PORTAIT_BOTTOM_RIGHT = 3
   , LANDSCAPE_TOP_LEFT = 4
   , LANDSCAPE_TOP_RIGHT = 5
   , LANDSCAPE_BOTTOM_LEFT = 6
   , LANDSCAPE_BOTTOM_RIGHT = 7
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
