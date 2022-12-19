#pragma once

#include "display_device.h"

namespace libesp {

class SPIDevice;
class SPIBus;
class FrameBuf;

/*
 * @author cmdc0de
 *RGBColor
 */
class EPDisplay: public DisplayDevice {
public:
   enum DISPLAY_TYPE {
      EP2_9 // epaper display 2.9 inch waveform
   };
   enum PIXEL_FORMAT {
		FORMAT_1_BIT 
	};
   static constexpr uint8_t COMMAND_DRIVER_OUTPUT = 0x1;
   static constexpr uint8_t COMMAND_SLEEP = 0x10;
   enum SLEEP_PARAMS {
      NORMAL_MODE = 0x00
         , DEEP_SLEEP_MODE = 0x01
   };
   static constexpr uint8_t COMMAND_DATA_ENTRY = 0x11;
   enum DATA_ENTRY_DIRECTION {
      X_COUNT_Y_DEC_X_DEC = 0b000
         , X_COUNT_Y_DEC_X_INC = 0b001
         , X_COUNT_Y_INC_X_DEC = 0b010
         , X_COUNT_Y_INC_X_INC = 0b011
         , Y_COUNT_Y_DEC_X_DEC = 0b100
         , Y_COUNT_Y_DEC_X_INC = 0b101
         , Y_COUNT_Y_INC_X_DEC = 0b110
         , Y_COUNT_Y_INC_X_INC = 0b111
   };
   static constexpr uint8_t COMMAND_SW_RESET = 0x12;
   static constexpr uint8_t COMMAND_TEMP_SET = 0x1A;
   static constexpr uint8_t COMMAND_MASTER_ACTIVATION = 0x20;
   static constexpr uint8_t COMMAND_DISPLAY_SETUP = 0x22;
   static constexpr uint8_t COMMAND_WRITE_RAM = 0x24;
   static constexpr uint8_t COMMAND_WRITE_LUT = 0x32;
   static constexpr uint8_t COMMAND_SET_RAM_X = 0x44;
   static constexpr uint8_t COMMAND_SET_RAM_Y = 0x45;
public:
  //inits SPI Bus as well as display
	static ErrorType initDisplay(gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, 
		int channel, gpio_num_t dataCmdPin, gpio_num_t resetPin, gpio_num_t busyPin, spi_host_device_t spiNum );
  // just inits display and NOT SPI bus
  static ErrorType initDisplay(gpio_num_t dataCmdPin, gpio_num_t resetPin, gpio_num_t busyPin );
public:
	EPDisplay(uint16_t w, uint16_t h, ROTATION r, gpio_num_t bl, gpio_num_t reset, RGBColor dt);
	ErrorType init(const FontDef_t *defaultFont, FrameBuf *);
	virtual ~EPDisplay();
	virtual bool drawPixel(int16_t x0, int16_t y0, const RGBColor &color);
	virtual void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	virtual void drawRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	virtual void fillScreen(const RGBColor &color);
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dcImage);
	void setMemoryAccessControl();

	virtual void swap();
	virtual void swap(bool block);
	virtual void reset();
   void setBackLightOn(bool on);
	///////////////////////////////////////////////////////
	virtual void drawVerticalLine(int16_t x,int16_t y,int16_t h);
	virtual void drawVerticalLine(int16_t x,int16_t y,int16_t h,const RGBColor &color);
	virtual void drawHorizontalLine(int16_t x, int16_t y, int16_t w);
	virtual void drawHorizontalLine(int16_t x,int16_t y,int16_t w,const RGBColor &color);
	//xPos and yPos are the pixel offsets, for each character drawn xPos is increased by the width of the current font
	virtual uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt);
	virtual uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor);
	virtual uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor, const RGBColor &bgColor, uint8_t size,	bool lineWrap);
	virtual uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor, const RGBColor &backGroundColor, uint8_t size, bool lineWrap, uint8_t charsToRender);
	virtual uint32_t drawStringOnLine(uint8_t line, const char *msg);
	//x and y are pixel locations
	virtual void drawCharAtPosition(int16_t x, int16_t y, char c,
			const RGBColor &textColor, const RGBColor &bgColor, uint8_t size);
	void setTextColor(const RGBColor &t);
	void setBackgroundColor(const RGBColor &t);
	const RGBColor &getTextColor();
	const RGBColor &getBackgroundColor();
  const DISPLAY_TYPE &getDisplayType() const {return DisplayType;}
private:
	RGBColor CurrentTextColor;
	RGBColor CurrentBGColor;
	gpio_num_t Reset;
   DISPLAY_TYPE DisplayType;
	gpio_num_t BackLight;
   uint8_t MemoryAccessControl;
	gpio_num_t BusyPin;
};

}
