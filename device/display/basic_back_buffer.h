#pragma once

#include <cstdint>
#include "../../error_type.h"
#include "color.h"
#include "display_types.h"


namespace libesp {

class BasicBackBuffer {
public:
	static const char *LOGTAG;
public:
	BasicBackBuffer(uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel, uint8_t *buffer
         , uint32_t backBufferSize, LIB_PIXEL_FORMAT pf);
	~BasicBackBuffer();
   ErrorType init();
	void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	void drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color);
	void drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color);
	bool drawPixel(int16_t x0, int16_t y0, const RGBColor &color);
	void drawImage(int16_t x, int16_t y, const DCImage &dc);
	void setPixelFormat(LIB_PIXEL_FORMAT pf);
	LIB_PIXEL_FORMAT getPixelFormat() const { return PixelFormat; }
	uint16_t getBufferWidth() const {return BufferWidth;}
	uint16_t getBufferHeight() const {return BufferHeight;}
	uint16_t getBitsPerPixelBuffer() const {return BitsPerPixelBuffer;}
   const uint8_t *getBackBuffer() const {return BackBuffer;}
   uint32_t getBackBufferSize() const {return BackBufferSize;}
protected:
   void placeColorInBuffer(uint16_t pixel, const RGBColor &color);
   void placeColorInBuffer(uint16_t pixel, const ColorPacker &pc);
private:
	LIB_PIXEL_FORMAT PixelFormat;
	uint16_t BufferWidth;
	uint16_t BufferHeight;
	uint8_t BitsPerPixelBuffer;
   uint8_t *BackBuffer;
   uint32_t BackBufferSize;
};

}
