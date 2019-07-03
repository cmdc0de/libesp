/*
 * frame_buffer.h
 *
 *  Created on: Jun 29, 2019
 *      Author: cmdc0de
 */

#ifndef LIBESP_DEVICE_DISPLAY_FRAME_BUFFER_H_
#define LIBESP_DEVICE_DISPLAY_FRAME_BUFFER_H_

#include <stdint.h>
#include <libesp/error_type.h>
#include <driver/gpio.h>
#include "color.h"
#include "../../utility/bitarray.h"

namespace libesp {

class SPIDevice;
class DisplayILI9341;
class SPIBus;

/*
 * Allows us to have different frame buffer types to make optimal use of memory
 * Current implemented (see below)
 * pass though
 * 2d buffer
 * 3d buffer
 */
class FrameBuf {
public:
	static const char *LOGTAG;
public:
	FrameBuf(DisplayILI9341 *d,uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel,uint16_t screenSizeX, uint16_t screenSizeY);
	ErrorType createInitDevice(SPIBus *bus, gpio_num_t cs, gpio_num_t data_cmd);
	virtual ~FrameBuf() { }
	virtual void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color)=0;
	virtual void drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color)=0;
	virtual void drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color)=0;
	virtual void swap()=0;
	virtual bool drawPixel(uint16_t x0, uint16_t y0, const RGBColor &color)=0;
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dc)=0;
	void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
	bool writeCmd(uint8_t c);
	bool writeNData(const uint8_t *data, int nbytes);
	bool write16Data(const uint16_t &data);
	bool writeN(char dc, const uint8_t *data, int nbytes);
	/////////////////////////////////////////
	// uint8_t to save space format is one of the value in the PIXEL_FORMAT ENUM
	void setPixelFormat(uint8_t pf);
	uint8_t getPixelFormat() const {
		return PixelFormat;
	}
	DisplayILI9341 *getDisplay() {return Display;}
	SPIDevice *getSPIDevice() {return SPI;}
	uint16_t getBufferWidth() const {return BufferWidth;}
	uint16_t getBufferHeight() const {return BufferHeight;}
	uint16_t getScreenWidth() const {return ScreenWidth;}
	uint16_t getScreenHeight() const {return ScreenHeight;}
private:
	DisplayILI9341 *Display;
	uint8_t PixelFormat;
	SPIDevice *SPI;
	uint16_t BufferWidth;
	uint16_t BufferHeight;
	uint16_t ScreenWidth;
	uint16_t ScreenHeight;
	uint8_t BitsPerPixel;
};


/*
 * @author cmdc0de
 * @date 6/2/17
 * Pass through frame buffer, basically no buffer every pixel goes
 * directly to SPI bus
 */
/*
class DrawBufferNoBuffer: public FrameBuf {
public:
	DrawBufferNoBuffer(DisplayST7735 *d, uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel,uint16_t screenSizeX, uint16_t *optimizedFillBuf, uint8_t rowsForDrawBuffer);
	virtual bool drawPixel(uint16_t x0, uint16_t y0, const RGBColor &color);
	virtual void drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color);
	virtual void drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color);
	virtual void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	virtual void swap();
	virtual ~DrawBufferNoBuffer();
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dc);
private:
	uint16_t *SPIBuffer;
	uint8_t RowsForDrawBuffer;
};
*/

/*
 * @author cmdc0de
 * @date 6/2/17
 *
 * 2 byte per pixel for backbuffer
 * 5 bits Red, 6 Bits Green, 5 bits blue -
 */
class ScalingBuffer : public FrameBuf {
public:
	//backbuf MUST be size of (bufferSizeX*BufferSizeY*bitsperpixel)/8
	ScalingBuffer(DisplayILI9341 *d, uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel, uint16_t screenSizeX, uint16_t screenSizeY, uint8_t rowsToBufferOut, uint8_t *backBuf, uint8_t *parallelLinesBuffer);
	virtual bool drawPixel(uint16_t x0, uint16_t y0, const RGBColor &color);
	virtual void drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color);
	virtual void drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color);
	virtual void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	//uses closest neighbor
	virtual void swap();
	virtual ~ScalingBuffer();
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dc);
protected:
	uint16_t calcLCDColor(const RGBColor &color);
private:
	uint8_t RowsToBufferOut;
	uint8_t *BackBuffer;
	uint8_t *ParallelLinesBuffer;
	float XRatio;
	float YRatio;
};

/*
 * @author cmdc0de
 * @date 6/2/17
 *
 * 1 byte per pixel for backbuffer
 * 3 bits Red, 2 Bits Green, 3 bits blue - color passed in is deresed from 565 to 323
 * On each pixel write we track if we changed any pixel in rowsForDrawBuffer,
 * if not on swap we skip those rows, otherwise write 2 bytes (565) to drawBuffer
 * for the width of the screen for 'rowsForDrawBuffer' rows. flush to SPI bus then repeat until
 * entire screen is rendered
 */

/*
class DrawBuffer2D16BitColor: public FrameBuf {
public:
	enum COLOR { // 2/2/2
		BLACK = 0,
		RED_MASK = 0x30,
		GREEN_MASK = 0xC,
		BLUE_MASK = 0x3,
		WHITE = 0x3F,
		BITS_PER_PIXEL = 2
	};
public:
	DrawBuffer2D16BitColor(uint16_t w, uint16_t h, uint8_t *backBuffer, uint8_t rowsForDrawBuffer,
			uint8_t *DrawBlocksBuf, DisplayST7735 *d);
	virtual ~DrawBuffer2D16BitColor();
	virtual bool drawPixel(uint16_t x, uint16_t y, const RGBColor &color);
	virtual void drawVerticalLine(int16_t x, int16_t y, int16_t h,
			const RGBColor &color);
	virtual void drawHorizontalLine(int16_t x, int16_t y, int16_t w,
			const RGBColor& color);
	virtual void fillRec(int16_t x, int16_t y, int16_t w, int16_t h,
			const RGBColor &color);
	virtual void swap();
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dc);
protected:
	uint16_t calcLCDColor(uint8_t packedColor);
	uint8_t deresColor(const RGBColor &color);
private:
	uint16_t Width;
	uint16_t Height;
	uint16_t BufferSize;
	BitArray BackBuffer;
	uint8_t RowsForDrawBuffer;
	BitArray DrawBlocksChanged;
};

*/
/*
 * @author cmdc0de
 * @date 6/2/17
 *
 * 2 byte per pixel for backbuffer
 * 5 bits Red, 6 Bits Green, 5 bits blue -
 * for a 128x160 lcd you'll need 40,960 bytes for the buffer
 */
/*
class DrawBuffer2D16BitColor16BitPerPixel1Buffer: public FrameBuf {
public:
	enum COLOR { // 2/2/2
		BLACK = 0,
		RED_MASK = 0x30,
		GREEN_MASK = 0xC,
		BLUE_MASK = 0x3,
		WHITE = 0xFFFF,
		BITS_PER_PIXEL = 5
	};
public:
	DrawBuffer2D16BitColor16BitPerPixel1Buffer(uint16_t w, uint16_t h, uint16_t *spiBuffer, DisplayST7735 *d);
	virtual ~DrawBuffer2D16BitColor16BitPerPixel1Buffer();
	virtual bool drawPixel(uint16_t x, uint16_t y, const RGBColor &color);
	virtual void drawVerticalLine(int16_t x, int16_t y, int16_t h,
			const RGBColor &color);
	virtual void drawHorizontalLine(int16_t x, int16_t y, int16_t w,
			const RGBColor& color);
	virtual void fillRec(int16_t x, int16_t y, int16_t w, int16_t h,
			const RGBColor &color);
	virtual void swap();
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dc);
protected:
	uint16_t calcLCDColor(const RGBColor &color);
private:
	uint16_t Width;
	uint16_t Height;
	uint16_t BufferSize;
	uint16_t *SPIBuffer;
};
*/
}



#endif /* COMPONENTS_LIBESP_DEVICE_DISPLAY_FRAME_BUFFER_H_ */
