/*
 * frame_buffer.h
 *
 *  Created on: Jun 29, 2019
 *      Author: cmdc0de
 */

#ifndef LIBESP_DEVICE_DISPLAY_FRAME_BUFFER_H_
#define LIBESP_DEVICE_DISPLAY_FRAME_BUFFER_H_

#include <stdint.h>
#include "../../error_type.h"
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
	virtual bool drawPixel(int16_t x0, int16_t y0, const RGBColor &color)=0;
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dc)=0;
	void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
	bool writeCmd(uint8_t c);
	bool writeNData(const uint8_t *data, int nbytes);
	bool write16Data(const uint16_t &data);
	bool writeN(char dc, const uint8_t *data, int nbytes);
	/////////////////////////////////////////
	// uint8_t to save space format is one of the value in the PIXEL_FORMAT ENUM
	void setPixelFormat(PackedColor::PIXEL_FORMAT pf);
	PackedColor::PIXEL_FORMAT getPixelFormat() const {
		return PixelFormat;
	}
	DisplayILI9341 *getDisplay() {return Display;}
	SPIDevice *getSPIDevice() {return SPI;}
	uint16_t getBufferWidth() const {return BufferWidth;}
	uint16_t getBufferHeight() const {return BufferHeight;}
	uint16_t getScreenWidth() const {return ScreenWidth;}
	uint16_t getScreenHeight() const {return ScreenHeight;}
	uint16_t getBitsPerPixelBuffer() const {return BitsPerPixelBuffer;}
private:
	DisplayILI9341 *Display;
	PackedColor::PIXEL_FORMAT PixelFormat;
	SPIDevice *SPI;
	uint16_t BufferWidth;
	uint16_t BufferHeight;
	uint16_t ScreenWidth;
	uint16_t ScreenHeight;
	uint8_t BitsPerPixelBuffer;
};

/*
 * @author cmdc0de
 * @date 6/2/19
 *
 * 2 byte per pixel for backbuffer
 * 5 bits Red, 6 Bits Green, 5 bits blue -
 */
class ScalingBuffer : public FrameBuf {
public:
	//backbuf MUST be size of (bufferSizeX*BufferSizeY*bitsperpixel)/8
	ScalingBuffer(DisplayILI9341 *d, uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel, uint16_t screenSizeX, uint16_t screenSizeY, uint8_t rowsToBufferOut, uint8_t *backBuf, uint8_t *parallelLinesBuffer);
	virtual bool drawPixel(int16_t x0, int16_t y0, const RGBColor &color);
	virtual void drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color);
	virtual void drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color);
	virtual void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	//uses closest neighbor
	virtual void swap();
	virtual ~ScalingBuffer();
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dc);
protected:
	void placeColorInBuffer(uint16_t pixel, uint8_t *buf, const RGBColor &color);
	void placeColorInBuffer(uint16_t pixel, uint8_t *buf, const PackedColor &pc);
private:
	uint8_t RowsToBufferOut;
	uint8_t *BackBuffer;
	uint8_t *ParallelLinesBuffer;
	float XRatio;
	float YRatio;
};

}

#endif /* COMPONENTS_LIBESP_DEVICE_DISPLAY_FRAME_BUFFER_H_ */
