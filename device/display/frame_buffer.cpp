/*Screen
 * frame_buffer.cpp
 *
 *  Created on: Jun 29, 2019
 *      Author: cmdc0de
 */


#include "frame_buffer.h"
#include "display_device.h"
#include <libesp/spibus.h>
#include <libesp/spidevice.h>


using namespace libesp;


FrameBuf::FrameBuf(DisplayST7735 *d,uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel,uint16_t screenSizeX, uint16_t screenSizeY)
	: Display(d), PixelFormat(0), SPI(0), BufferWidth(bufferSizeX), BufferHeight(bufferSizeY), ScreenWidth(screenSizeX), ScreenHeight(screenSizeY), BitsPerPixel(bitsPerPixel) {

}


//TODO FIX THIS UGLY HACK
static gpio_num_t DATA_CMD_PIN;
//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void spi_pre_cb(spi_transaction_t *t) {
	int dc=(int)t->user;
	gpio_set_level(DATA_CMD_PIN, dc);
}

ErrorType FrameBuf::createInitDevice(SPIBus *bus, gpio_num_t cs, gpio_num_t data_cmd) {
	spi_device_interface_config_t devcfg;
	memset(&devcfg,0,sizeof(devcfg));
	//this could be calculated!!!
	devcfg.clock_speed_hz=10*1000*1000;         //Clock out at 10 MHz
	devcfg.mode=0;          //SPI mode 0
	devcfg.spics_io_num=cs; //CS pin
	DATA_CMD_PIN = data_cmd;
	//TODO This should be calculated based on SPIBus buffer size and number of
	//back buffer pizels
	devcfg.queue_size=3; //We want to be able to queue 3 transactions at a time
	devcfg.duty_cycle_pos = 0;
	devcfg.cs_ena_pretrans = 0;
	devcfg.cs_ena_posttrans = 0;
	devcfg.input_delay_ns = 0;
	devcfg.flags = 0;
	devcfg.pre_cb = spi_pre_cb;
	devcfg.post_cb = nullptr;

	SPI = bus->createMasterDevice(devcfg);
	if(!SPI) {
		return ErrorType();
	}
	return ErrorType();
}

///////////////////////////////////////////////////////////////////////
void FrameBuf::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	//if ((MemoryAccessControl & DisplayST7735::MADCTL_MY) == 0) {
	//	writeCmd(DisplayST7735::COLUMN_ADDRESS_SET);
	//	write16Data(y0);
	//	write16Data(y1);

	//	writeCmd(DisplayST7735::ROW_ADDRESS_SET);
	//	write16Data(x0);
	//	write16Data(x1);
	//} else {
	//this code thinks about everything as x is columsn y is rows
		writeCmd(DisplayST7735::COLUMN_ADDRESS_SET);
		write16Data(x0);
		write16Data(x1);

		writeCmd(DisplayST7735::ROW_ADDRESS_SET);
		write16Data(y0);
		write16Data(y1);
	//}
}


void FrameBuf::setPixelFormat(uint8_t pf) {
	if (PixelFormat != pf) {
		PixelFormat = pf;
		writeCmd(DisplayST7735::INTERFACE_PIXEL_FORMAT);
		writeNData(&pf, 1);
	}
}


bool FrameBuf::writeCmd(uint8_t c) {
	return writeN(0, &c, sizeof(c));
}

bool FrameBuf::writeNData(const uint8_t *data, int nbytes) {
	return writeN(1, data, nbytes);
}

bool FrameBuf::writeN(char dc, const uint8_t *data, int nbytes) {
	void *ud = (void*)dc;
	SPI->send(data,nbytes,ud);
	return true;
}

bool FrameBuf::write16Data(const uint16_t &data) {
	uint8_t buf[2];
	buf[0] = data >> 8;
	buf[1] = data & 0xFF;
	return writeN(1, &buf[0], sizeof(buf));
}

//Scaling buffer

ScalingBuffer::ScalingBuffer(DisplayST7735 *d, uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel, uint16_t screenSizeX, uint16_t screenSizeY, uint8_t rowsToBufferOut, uint8_t *backBuf, uint8_t *parallelLinesBuffer)
	: FrameBuf(d,bufferSizeX, bufferSizeY, bitsPerPixel, screenSizeX, screenSizeY), RowsToBufferOut(rowsToBufferOut), BackBuffer(backBuf), ParallelLinesBuffer(parallelLinesBuffer) {

}

uint16_t ScalingBuffer::calcLCDColor(const RGBColor &color) {
	uint32_t rc = color.getR()/8; //keep it in 5 bits
	uint32_t gc = color.getG()/4; //keep it in 6 bits
	uint32_t bc = color.getB()/8; //keep it in 5 bits
	RGBColor lcdColor(rc, gc, bc);
	uint8_t *packedData = DisplayST7735::PackedColor::create(getPixelFormat(), lcdColor).getPackedColorData();
	uint16_t *pcd = (uint16_t*)packedData;
	return *pcd;
}

bool ScalingBuffer::drawPixel(uint16_t x0, uint16_t y0, const RGBColor &color) {
	BackBuffer[(y0*getBufferWidth())+x0] = calcLCDColor(color);
	return true;
}


ScalingBuffer::~ScalingBuffer() {

}

//TODO BOUNDS CHECK!
void ScalingBuffer::drawImage(int16_t x1, int16_t y1, const DCImage &dc) {
	uint16_t *t = (uint16_t*)&dc.pixel_data[0];
	for(int y=0;y<dc.height;++y) {
		for(int x=0;x<dc.width;++x) {
			RGBColor c(((t[(y*dc.height)+x]&0b1111100000000000)>>11),
					   ((t[(y*dc.height)+x]&0b0000011111100000)>>5),
					   ((t[(y*dc.height)+x]&0b0000000000011111)));
			uint8_t *packedData = DisplayST7735::PackedColor::create(getPixelFormat(), c).getPackedColorData();
			uint16_t *pcd = (uint16_t*)packedData;
			BackBuffer[((y+y1)*getBufferWidth())+(x+x1)] = (*pcd);
		}
	}
}

//TODO bounds check and bitsperpixlel check
void ScalingBuffer::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	uint16_t c = calcLCDColor(color);
	for (int i = y; i < (h + y); ++i) {
		uint32_t offset = i * getBufferWidth();
		for(int j=0;j<w;++j) {
			BackBuffer[offset+x+j] = c;
		}
	}
}

//TODO bounds check and bitsperpixlel check
void ScalingBuffer::drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color) {
	uint16_t c = calcLCDColor(color);
	for (int i = y; i < (h + y); ++i) {
		BackBuffer[i * getBufferWidth() + x] = c;
	}
}

void ScalingBuffer::drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color) {
	uint16_t c = calcLCDColor(color);
	uint32_t offset = y*getBufferWidth();
	for(int i=x;i<(x+w);++i) {
		BackBuffer[offset+i] = c;
	}
}

//TODO BUG if RowsToBufferOut isn't a even divisor of ScreenHeight
void ScalingBuffer::swap() {
	if( 1) { //anychange
		setAddrWindow(0, 0, getScreenWidth(), getScreenHeight());
		writeCmd(DisplayST7735::MEMORY_WRITE);
		uint16_t totalLines = 0;
		while(totalLines<this->getScreenHeight()) {
			//2 because 16 bits per pixel...fix this to be more geneic
			memset(&ParallelLinesBuffer[0],0, RowsToBufferOut*this->getBufferWidth()*2);
			for(int i=0;i<this->RowsToBufferOut;++i) {
				memcpy(&ParallelLinesBuffer[i*this->getScreenWidth()*2],&BackBuffer[totalLines*this->getBufferWidth()*2], this->getBufferWidth()*2);
			}
			totalLines+=RowsToBufferOut;
			writeNData(&ParallelLinesBuffer[0],RowsToBufferOut*this->getBufferWidth()*2);
		}
	}
}


/*
//////////////////////////////////////////////////////////////////////
DrawBufferNoBuffer::DrawBufferNoBuffer(DisplayST7735 *d, uint16_t *optimizedFillBuf, uint8_t rowsForDrawBuffer) : FrameBuf(d),
		SPIBuffer(optimizedFillBuf), RowsForDrawBuffer(rowsForDrawBuffer) {
}

bool DrawBufferNoBuffer::drawPixel(uint16_t x0, uint16_t y0, const RGBColor &color) {
	DisplayST7735::PackedColor pc = DisplayST7735::PackedColor::create(getPixelFormat(), color);
	setAddrWindow(x0, y0, x0, y0);
	writeCmd(DisplayST7735::MEMORY_WRITE);
	return writeNData(pc.getPackedColorData(), pc.getSize());
}

void DrawBufferNoBuffer::drawImage(int16_t x, int16_t y, const DCImage &dc) {
	setAddrWindow(0,0,dc.width,dc.height);
	writeCmd(DisplayST7735::MEMORY_WRITE);
	writeNData((const uint8_t*)&dc.pixel_data[0],dc.height*dc.width*dc.bytes_per_pixel);
}

void DrawBufferNoBuffer::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	setAddrWindow(x, y, w, h);
	writeCmd(DisplayST7735::MEMORY_WRITE);
	DisplayST7735::PackedColor pc = DisplayST7735::PackedColor::create(getPixelFormat(), color);

	uint16_t pcolor = *((uint16_t*) (pc.getPackedColorData()));
	uint16_t pixelCount = w * h;
	uint16_t maxAtOnce =
			pixelCount > (RowsForDrawBuffer * getDisplay()->getWidth()) ?
					(RowsForDrawBuffer * getDisplay()->getWidth()) : pixelCount;
	for (uint16_t i = 0; i < maxAtOnce; ++i) {
		SPIBuffer[i] = pcolor;
	}

	uint16_t pixelCopied = 0;
	do {
		writeNData((uint8_t*) &SPIBuffer[0], maxAtOnce * sizeof(uint16_t));
		pixelCopied += maxAtOnce;

		if ((pixelCopied + maxAtOnce) > pixelCount) {
			maxAtOnce = pixelCount - pixelCopied;
		}
	} while (pixelCopied < pixelCount);
}

void DrawBufferNoBuffer::drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color) {
	DisplayST7735::PackedColor pc = DisplayST7735::PackedColor::create(getPixelFormat(), color);

	uint16_t pcolor = *((uint16_t*) (pc.getPackedColorData()));
	uint16_t pixelCount = h;
	uint16_t maxAtOnce =
			pixelCount > (RowsForDrawBuffer * getDisplay()->getWidth()) ?
					(RowsForDrawBuffer * getDisplay()->getWidth()) : pixelCount;
	for (uint16_t i = 0; i < maxAtOnce; ++i) {
		SPIBuffer[i] = pcolor;
	}

	uint16_t pixelCopied = 0;
	setAddrWindow(x, y, x, y + h - 1);
	writeCmd(DisplayST7735::MEMORY_WRITE);
	do {
		writeNData((uint8_t*) &SPIBuffer[0], maxAtOnce * sizeof(uint16_t));
		pixelCopied += maxAtOnce;

		if ((pixelCopied + maxAtOnce) > pixelCount) {
			maxAtOnce = pixelCount - pixelCopied;
		}
	} while (pixelCopied < pixelCount);
}

void DrawBufferNoBuffer::drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color) {
	DisplayST7735::PackedColor pc = DisplayST7735::PackedColor::create(getPixelFormat(), color);

	uint16_t pcolor = *((uint16_t*) (pc.getPackedColorData()));
	for (uint16_t i = 0; i < w; ++i) {
		SPIBuffer[i] = pcolor;
	}

	setAddrWindow(x, y, x + w - 1, y);
	writeCmd(DisplayST7735::MEMORY_WRITE);
	writeNData((uint8_t*) &SPIBuffer[0], w * sizeof(uint16_t));
}

void DrawBufferNoBuffer::swap() {
	//do nothing
}

DrawBufferNoBuffer::~DrawBufferNoBuffer() {

}
*/
/*

DrawBuffer2D16BitColor::DrawBuffer2D16BitColor(uint16_t w, uint16_t h,
	uint8_t *backBuffer, uint8_t rowsForDrawBuffer,
	uint8_t *drawBlocksBuffer, DisplayST7735 *d)
		  : FrameBuf(d), Width(w), Height(h),
		  BufferSize(w * h), BackBuffer(backBuffer,w*h,6),
		  RowsForDrawBuffer( rowsForDrawBuffer), DrawBlocksChanged(drawBlocksBuffer,h/rowsForDrawBuffer,1) {
}

DrawBuffer2D16BitColor::~DrawBuffer2D16BitColor() {

}

bool DrawBuffer2D16BitColor::drawPixel(uint16_t x, uint16_t y, const RGBColor &color) {
	uint8_t c = deresColor(color);
	BackBuffer.setValueAsByte((y * Width) + x,c);
	DrawBlocksChanged.setValueAsByte(y / RowsForDrawBuffer,1);
	return true;
}

//not using buffer just write directly to SPI
void DrawBuffer2D16BitColor::drawImage(int16_t x, int16_t y, const DCImage &dc) {
	setAddrWindow(0,0,dc.width-1,dc.height-1);
	writeCmd(DisplayST7735::MEMORY_WRITE);
	writeNData((const uint8_t*)&dc.pixel_data[0],dc.height*dc.width*dc.bytes_per_pixel);
	DrawBlocksChanged.clear();
}

void DrawBuffer2D16BitColor::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	uint8_t c = deresColor(color);
	for (int i = y; i < (h + y); ++i) {
		//OPTIMIZE THIS BY MAKING A SET RANGE IN BITARRAY
		uint32_t offset = i * getDisplay()->getWidth();
		for(int j=0;j<w;++j) {
			BackBuffer.setValueAsByte(offset+x+j, c);
		}
		DrawBlocksChanged.setValueAsByte(i / RowsForDrawBuffer,1);
	}
}

void DrawBuffer2D16BitColor::drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color) {
	uint8_t c = deresColor(color);
	for (int i = y; i < (h + y); ++i) {
		BackBuffer.setValueAsByte(i * getDisplay()->getWidth() + x,c);
		DrawBlocksChanged.setValueAsByte(i / RowsForDrawBuffer,1);
	}
}

void DrawBuffer2D16BitColor::drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color) {
	uint8_t c = deresColor(color);
	uint32_t offset = y*getDisplay()->getWidth();
	for(int i=x;i<(x+w);++i) {
		BackBuffer.setValueAsByte(offset+i, c);
	}
	DrawBlocksChanged.setValueAsByte(y/RowsForDrawBuffer,1);
}

//////
// first check to see if we changed anything in the draw block, if not skip it
// if we did change something convert from our short hand notation to something the LCD will understand
//	then send to LCD
void DrawBuffer2D16BitColor::swap() {
	for (int h = 0; h < Height; h++) {
		if ((DrawBlocksChanged.getValueAsByte(h / RowsForDrawBuffer)) != 0) {
			for (int w = 0; w < Width; w++) {
				uint32_t SPIY = h % RowsForDrawBuffer;
				//SPIBuffer[(SPIY * Width) + w] = calcLCDColor(BackBuffer.getValueAsByte((h * Width) + w));
			}
			if (h != 0 && (h % RowsForDrawBuffer == (RowsForDrawBuffer-1))) {
				setAddrWindow(0, h - (RowsForDrawBuffer-1), Width, h);
				writeCmd(DisplayST7735::MEMORY_WRITE);
				//writeNData((uint8_t*) &SPIBuffer[0], Width*RowsForDrawBuffer*sizeof(uint16_t));
			}
		}
	}
	DrawBlocksChanged.clear();
}

uint16_t DrawBuffer2D16BitColor::calcLCDColor(uint8_t packedColor) {
	static const uint8_t colorValues[4] = {0,85,170,255};
	uint32_t rc = (packedColor & RED_MASK) >> 4;
	uint32_t gc = (packedColor & GREEN_MASK) >> 2;
	uint32_t bc = packedColor & BLUE_MASK;
	RGBColor lcdColor(colorValues[rc], colorValues[gc], colorValues[bc]);
	uint8_t *packedData = DisplayST7735::PackedColor::create(getPixelFormat(), lcdColor).getPackedColorData();
	uint16_t *pcd = (uint16_t*)packedData;
	return *pcd;
}
uint8_t DrawBuffer2D16BitColor::deresColor(const RGBColor &color) {
	uint32_t retVal = 0;
	retVal = (color.getR() / 85) << 4;
	retVal |= (color.getG() / 85) << 2;
	retVal |= (color.getB() / 85);
	return retVal;
}


/////////////////////////////////////////

DrawBuffer2D16BitColor16BitPerPixel1Buffer::DrawBuffer2D16BitColor16BitPerPixel1Buffer(uint16_t w, uint16_t h, uint16_t *spiBuffer, DisplayST7735 *d)
	: FrameBuf(d), Width(w), Height(h), BufferSize(w * h * sizeof(uint16_t)), SPIBuffer(spiBuffer) {
}

DrawBuffer2D16BitColor16BitPerPixel1Buffer::~DrawBuffer2D16BitColor16BitPerPixel1Buffer() {

}

bool DrawBuffer2D16BitColor16BitPerPixel1Buffer::drawPixel(uint16_t x, uint16_t y, const RGBColor &color) {
	SPIBuffer[(y*Width)+x] = calcLCDColor(color);
	return true;
}

//not using buffer just write directly to SPI
void DrawBuffer2D16BitColor16BitPerPixel1Buffer::drawImage(int16_t x1, int16_t y1, const DCImage &dc) {
#if 0
	setAddrWindow(0,0,dc.width,dc.height);
	writeCmd(DisplayST7735::MEMORY_WRITE);
	writeNData((const uint8_t*)&dc.pixel_data[0],dc.height*dc.width*dc.bytes_per_pixel);
#else
	uint16_t *t = (uint16_t*)&dc.pixel_data[0];
	for(int y=0;y<dc.height;++y) {
		for(int x=0;x<dc.width;++x) {
			RGBColor c(((t[(y*dc.height)+x]&0b1111100000000000)>>11),
					   ((t[(y*dc.height)+x]&0b0000011111100000)>>5),
					   ((t[(y*dc.height)+x]&0b0000000000011111)));
			uint8_t *packedData = DisplayST7735::PackedColor::create(getPixelFormat(), c).getPackedColorData();
			uint16_t *pcd = (uint16_t*)packedData;
			SPIBuffer[((y+y1)*Width)+(x+x1)] = (*pcd);
		}
	}
#endif
}

void DrawBuffer2D16BitColor16BitPerPixel1Buffer::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	uint16_t c = calcLCDColor(color);
	for (int i = y; i < (h + y); ++i) {
		//OPTIMIZE THIS BY MAKING A SET RANGE IN BITARRAY
		uint32_t offset = i * getDisplay()->getWidth();
		for(int j=0;j<w;++j) {
			SPIBuffer[offset+x+j] = c;
		}
	}
}

void DrawBuffer2D16BitColor16BitPerPixel1Buffer::drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color) {
	uint16_t c = calcLCDColor(color);
	for (int i = y; i < (h + y); ++i) {
		SPIBuffer[i * getDisplay()->getWidth() + x] = c;
	}
}

void DrawBuffer2D16BitColor16BitPerPixel1Buffer::drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color) {
	uint16_t c = calcLCDColor(color);
	uint32_t offset = y*getDisplay()->getWidth();
	for(int i=x;i<(x+w);++i) {
		SPIBuffer[offset+i] = c;
	}
}

void DrawBuffer2D16BitColor16BitPerPixel1Buffer::swap() {
	if( 1) { //anychange
		setAddrWindow(0, 0, Width, Height);
		writeCmd(DisplayST7735::MEMORY_WRITE);
		writeNData((uint8_t*) &SPIBuffer[0], BufferSize);
	}
}

uint16_t DrawBuffer2D16BitColor16BitPerPixel1Buffer::calcLCDColor(const RGBColor &color) {
	uint32_t rc = color.getR()/8; //keep it in 5 bits
	uint32_t gc = color.getG()/4; //keep it in 6 bits
	uint32_t bc = color.getB()/8; //keep it in 5 bits
	RGBColor lcdColor(rc, gc, bc);
	uint8_t *packedData = DisplayST7735::PackedColor::create(getPixelFormat(), lcdColor).getPackedColorData();
	uint16_t *pcd = (uint16_t*)packedData;
	return *pcd;
}
*/

