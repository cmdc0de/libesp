
#include "basic_back_buffer.h"
#include "device/display/display_types.h"
#include <string.h>

using namespace libesp;


const char *BasicBackBuffer::LOGTAG = "BasicBackBuffer";

BasicBackBuffer::BasicBackBuffer(uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel
      , uint8_t *buffer, uint32_t backBufferSize, LIB_PIXEL_FORMAT pf) 
      : PixelFormat(pf), BufferWidth(bufferSizeX), BufferHeight(bufferSizeY)
      , BitsPerPixelBuffer(bitsPerPixel), BackBuffer(buffer), BackBufferSize(backBufferSize) {

   }
	
BasicBackBuffer::~BasicBackBuffer() { 
}
   
ErrorType BasicBackBuffer::init() {
   return ErrorType();
}

void BasicBackBuffer::placeColorInBuffer(uint16_t pixel, const RGBColor &color) {
	ColorPacker pc = ColorPacker::create2(getPixelFormat(), color);
	placeColorInBuffer(pixel,pc);
}

void BasicBackBuffer::placeColorInBuffer(uint16_t pixel, const ColorPacker &pc) {
	const uint8_t *packedData = pc.getPackedColorData();
	uint16_t bufferPos = pixel*(getBitsPerPixelBuffer()/8);
	for(int i=0;i<pc.getSize();++i) {
		BackBuffer[bufferPos+i]=packedData[i];
	}
}

void BasicBackBuffer::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	if(x<0||x>=getBufferWidth()||y<0||y>getBufferHeight()) return;

	ColorPacker pc = ColorPacker::create2(getPixelFormat(), color);
	for (int i = y; i < (h + y); ++i) {
		uint32_t offset = i * getBufferWidth();
		for(int j=x;j<(x+w);++j) {
			placeColorInBuffer(offset+j, pc);
		}
		//ESP_LOGI(LOGTAG,"%d",i);
		//ESP_LOG_BUFFER_HEX(LOGTAG,&BackBuffer[offset],getBufferWidth());
   }
}

void BasicBackBuffer::drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color) {
	if(x<0||x>=getBufferWidth()||y<0||y>getBufferHeight()) return;

	ColorPacker pc = ColorPacker::create2(getPixelFormat(), color);
	for (int i = y; i < (h + y); ++i) {
		placeColorInBuffer(i*getBufferWidth()+x, pc);
   } 
}

void BasicBackBuffer::drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color) {
	if(x<0||x>=getBufferWidth()||y<0||y>getBufferHeight()) return;

	ColorPacker pc = ColorPacker::create2(getPixelFormat(), color);
	uint32_t offset = y*getBufferWidth();
	for(int i=x;i<(x+w);++i) {
		placeColorInBuffer(offset+i, pc);
	}
}

bool BasicBackBuffer::drawPixel(int16_t x0, int16_t y0, const RGBColor &color) {
	if(x0<0||x0>=getBufferWidth()||y0<0||y0>getBufferHeight()) return false;

	placeColorInBuffer(y0*getBufferWidth()+x0, color);
	return true;
}

void BasicBackBuffer::drawImage(int16_t x1, int16_t y1, const DCImage &dc) {
	uint16_t *t = (uint16_t*)&dc.pixel_data[0];
   if(dc.bytes_per_pixel==2 && (getBitsPerPixelBuffer()/8)==2) {
	   for(int y=0;y<dc.height;++y) {
         if((y+y1)<getBufferHeight()) {
            uint16_t bufferPosStart = (((y+y1)*getBufferWidth())+x1)*2;
            memcpy(&BackBuffer[bufferPosStart],&t[(y*dc.width)],dc.width*2);
         }
      }
   } else if (dc.bytes_per_pixel==3) {
	   for(int y=0;y<dc.height;++y) {
		   for(int x=0;x<dc.width;++x) {
			   RGBColor c(((t[(y*dc.height)+x]&0b1111100000000000)>>11),
					   ((t[(y*dc.height)+x]&0b0000011111100000)>>5),
					   ((t[(y*dc.height)+x]&0b0000000000011111)));
			   ColorPacker pc = ColorPacker::create(getPixelFormat(), c);
			   const uint8_t *packedData = pc.getPackedColorData();
			   uint16_t bufferPos = (y+y1)*getBufferWidth()+(x+x1);
			   bufferPos = bufferPos*(getBitsPerPixelBuffer()/8);
			   for(int i=0;i<pc.getSize();++i) {
				   BackBuffer[bufferPos+i]=packedData[i];
			   }
		   }
	   }
   }
}

void BasicBackBuffer::setPixelFormat(LIB_PIXEL_FORMAT pf) {
   PixelFormat = pf;
}


