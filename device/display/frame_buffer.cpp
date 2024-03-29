/*Screen
 * frame_buffer.cpp
 *
 *  Created on: Jun 29, 2019
 *      Author: cmdc0de
 */


#include "frame_buffer.h"
#include "display_device.h"
#include "../../spibus.h"
#include "../../spidevice.h"
#include <esp_log.h>
#include <cstring>


using namespace libesp;

const char *FrameBuf::LOGTAG = "FrameBuf";

FrameBuf::FrameBuf(TFTDisplay *d,uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel,uint16_t screenSizeX, uint16_t screenSizeY)
	: Display(d), PixelFormat(PackedColor::PIXEL_FORMAT_16_BIT), SPI(0), BufferWidth(bufferSizeX), BufferHeight(bufferSizeY), ScreenWidth(screenSizeX), ScreenHeight(screenSizeY), BitsPerPixelBuffer(bitsPerPixel) {

}

//IDEA set t->user to negarive DATA_CMD_PIN for 0 and pos to 1
//TODO FIX THIS UGLY HACK
static gpio_num_t DATA_CMD_PIN;
//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void spi_pre_cb(spi_transaction_t *t) {
	int dc=reinterpret_cast<int>(t->user);
	gpio_set_level(DATA_CMD_PIN, dc);
}

ErrorType FrameBuf::createInitDevice(SPIBus *bus, gpio_num_t cs, gpio_num_t data_cmd) {
  return createInitDevice(bus,cs,data_cmd,nullptr);
}

ErrorType FrameBuf::createInitDevice(SPIBus *bus, gpio_num_t cs, gpio_num_t data_cmd, SemaphoreHandle_t handle) {
	ESP_LOGI(LOGTAG,"start createInitDevice");
	spi_device_interface_config_t devcfg;
	memset(&devcfg,0,sizeof(devcfg));
	//this could be calculated!!!
	devcfg.clock_speed_hz=10*1000*1000;         //Clock out at 10 MHz
	devcfg.mode=0;          //SPI mode 0
	devcfg.spics_io_num=cs; //CS pin
	DATA_CMD_PIN = data_cmd;
	//TODO This should be calculated based on SPIBus buffer size and number of
	//back buffer pizels
	devcfg.queue_size=2; //# of transactions at a time
	devcfg.duty_cycle_pos = 0;
	devcfg.cs_ena_pretrans = 0;
	devcfg.cs_ena_posttrans = 0;
	devcfg.input_delay_ns = 0;
	devcfg.flags = 0;
	devcfg.pre_cb = spi_pre_cb;
	devcfg.post_cb = nullptr;

	SPI = bus->createMasterDevice(devcfg, handle);
	if(!SPI) {
		ESP_LOGE(LOGTAG,"failed createInitDevice");
		//TODO FIXME
		return ErrorType();
	}
	//read id
	uint8_t cmdData = 0;
	uint8_t cmd = 0x4;
	SPI->send(&cmd,1,&cmdData);
	uint8_t buf[3] = {0};
	cmdData = 1;
	SPI->sendAndReceive(&buf[0],&buf[0],sizeof(buf),&cmdData);
	ESP_LOGI(LOGTAG,"DISPLAY ID");
	ESP_LOG_BUFFER_HEX(LOGTAG,&buf[0],sizeof(buf));
	return ErrorType();
}

///////////////////////////////////////////////////////////////////////
void FrameBuf::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	if (Display->getRotation()==DisplayDevice::PORTAIT_TOP_LEFT) {
		writeCmd(TFTDisplay::COLUMN_ADDRESS_SET);
		write16Data(y0);
		write16Data(y1);

		writeCmd(TFTDisplay::ROW_ADDRESS_SET);
		write16Data(x0);
		write16Data(x1);
	} else {
		writeCmd(TFTDisplay::COLUMN_ADDRESS_SET);
		write16Data(x0);
		write16Data(x1);

		writeCmd(TFTDisplay::ROW_ADDRESS_SET);
		write16Data(y0);
		write16Data(y1);
	}
}


void FrameBuf::setPixelFormat(PackedColor::PIXEL_FORMAT pf) {
	PixelFormat = pf;
}


bool FrameBuf::writeCmd(uint8_t c) {
	return writeN(0, &c, sizeof(c));
}

bool FrameBuf::writeNData(const uint8_t *data, int nbytes) {
	return writeN(1, data, nbytes);
}

bool FrameBuf::writeN(char dc, const uint8_t *data, int nbytes) {
	void *ud = reinterpret_cast<void*>(dc);
	ErrorType et = SPI->send(data,nbytes,ud);
	return et.ok();
}

bool FrameBuf::write16Data(const uint16_t &data) {
	uint8_t buf[2];
	buf[0] = data >> 8;
	buf[1] = data & 0xFF;
	return writeN(1, &buf[0], sizeof(buf));
}

//Scaling buffer
ScalingBuffer::ScalingBuffer(TFTDisplay *d, uint16_t bufferSizeX, uint16_t bufferSizeY, uint8_t bitsPerPixel, uint16_t screenSizeX, uint16_t screenSizeY, uint8_t rowsToBufferOut, uint8_t *backBuf, uint8_t *parallelLinesBuffer)
	: FrameBuf(d,bufferSizeX, bufferSizeY, bitsPerPixel, screenSizeX, screenSizeY), RowsToBufferOut(rowsToBufferOut), BackBuffer(backBuf), ParallelLinesBuffer(parallelLinesBuffer) {
	XRatio = float(getBufferWidth())/float(getScreenWidth());
	YRatio = float(getBufferHeight())/float(getScreenHeight());
}

void ScalingBuffer::placeColorInBuffer(uint16_t pixel, uint8_t *buf, const RGBColor &color) {
	PackedColor pc = PackedColor::create2(getPixelFormat(), color);
	placeColorInBuffer(pixel,buf,pc);
}

void ScalingBuffer::placeColorInBuffer(uint16_t pixel, uint8_t *buf, const PackedColor &pc) {
	const uint8_t *packedData = pc.getPackedColorData();
	uint16_t bufferPos = pixel*(getBitsPerPixelBuffer()/8);
	for(int i=0;i<pc.getSize();++i) {
		buf[bufferPos+i]=packedData[i];
	}
}

bool ScalingBuffer::drawPixel(int16_t x0, int16_t y0, const RGBColor &color) {
	//BackBuffer[(y0*getBufferWidth())+x0] = calcLCDColor(color);
	if(x0<0||x0>=getBufferWidth()||y0<0||y0>getBufferHeight()) return false;
	placeColorInBuffer(y0*getBufferWidth()+x0, &BackBuffer[0],color);
	return true;
}


ScalingBuffer::~ScalingBuffer() {

}

//TODO BOUNDS CHECK!
void ScalingBuffer::drawImage(int16_t x1, int16_t y1, const DCImage &dc) {
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
			   PackedColor pc = PackedColor::create(getPixelFormat(), c);
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

//TODO bounds check and bitsperpixlel check
void ScalingBuffer::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	if(x<0||x>=getBufferWidth()||y<0||y>getBufferHeight()) return;
	PackedColor pc = PackedColor::create2(getPixelFormat(), color);
	for (int i = y; i < (h + y); ++i) {
		uint32_t offset = i * getBufferWidth();
		for(int j=x;j<(x+w);++j) {
			placeColorInBuffer(offset+j, &BackBuffer[0],pc);
		}
		//ESP_LOGI(LOGTAG,"%d",i);
		//ESP_LOG_BUFFER_HEX(LOGTAG,&BackBuffer[offset],getBufferWidth());
	}
}

//TODO bounds check and bitsperpixlel check
void ScalingBuffer::drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color) {
	if(x<0||x>=getBufferWidth()||y<0||y>getBufferHeight()) return;
	PackedColor pc = PackedColor::create2(getPixelFormat(), color);
	for (int i = y; i < (h + y); ++i) {
		placeColorInBuffer(i*getBufferWidth()+x, &BackBuffer[0],pc);
		//BackBuffer[i * getBufferWidth() + x] = c;
	}
}

void ScalingBuffer::drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color) {
	if(x<0||x>=getBufferWidth()||y<0||y>getBufferHeight()) return;
	PackedColor pc = PackedColor::create2(getPixelFormat(), color);
	uint32_t offset = y*getBufferWidth();
	for(int i=x;i<(x+w);++i) {
		placeColorInBuffer(offset+i, &BackBuffer[0],pc);
	}
}

//TODO BUG if RowsToBufferOut isn't a even divisor of ScreenHeight
void ScalingBuffer::swap() {
	//ESP_LOGI(LOGTAG,"swap");
	setAddrWindow(0, 0, getScreenWidth()-1, getScreenHeight()-1);
	writeCmd(TFTDisplay::MEMORY_WRITE);
	uint16_t totalLines = 0;
	uint32_t newX, newY;
	uint16_t *source = (uint16_t*)&BackBuffer[0];
	uint16_t *target = (uint16_t*)&ParallelLinesBuffer[0];
	uint8_t *s,*t;
	//ESP_LOG_BUFFER_HEX(LOGTAG,&BackBuffer[0],12);
	while(totalLines<getScreenHeight()) {
		//2 because 16 bits per pixel...fix this to be more geneic
		memset(&ParallelLinesBuffer[0],0, RowsToBufferOut*getScreenWidth()*2);
		for(int y=0;y<RowsToBufferOut;++y) {
			for(int x=0;x<getScreenWidth();++x) {
				newX = ((x*XRatio));
				newY = ((y+totalLines)*YRatio);
				t = (uint8_t*)&target[(y*getScreenWidth())+x];
				s = (uint8_t*)&source[(newY*getBufferWidth())+newX];
				t[0] = s[0];
				t[1] = s[1];
			}
		}
		//ESP_LOG_BUFFER_HEX(LOGTAG,&ParallelLinesBuffer[0],RowsToBufferOut*getScreenWidth()*2);
		if(!writeNData(&ParallelLinesBuffer[0],RowsToBufferOut*this->getScreenWidth()*2)) {
			ESP_LOGE(LOGTAG,"***************** Error writing out frame buffer");
		}
		totalLines+=RowsToBufferOut;
	}
	//ESP_LOGI(LOGTAG,"end swap");
}


