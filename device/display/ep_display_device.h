#pragma once

#include "display_device.h"
#include <cstdint>

namespace libesp {

class SPIDevice;
class SPIBus;
struct sCmdBuf;


/*
 * @author cmdc0de
 */
class EPDisplay: public DisplayDevice {
public:
   enum DISPLAY_TYPE {
      EP2_9 = 0 // epaper display 2.9 inch waveform
      , TOTAL_DISPLAY_TYPES
   };
   enum PIXEL_FORMAT {
		FORMAT_1_BIT = 1
	};
   struct DisplaySpecs {
      uint16_t Width;
      uint16_t Height;
      uint32_t BackBufferSize;
      PIXEL_FORMAT PixelFormat;
      const sCmdBuf *initCmds;
      uint16_t NumCmds;
   };
   static const DisplaySpecs &getSpecs(const DISPLAY_TYPE &dt);
public:
   static constexpr const uint16_t EP29_WIDTH = 128;
   static constexpr const uint16_t EP29_HEIGHT = 296;
   static constexpr const uint32_t EP29_BK_BUFFER_SIZE = (EP29_WIDTH/8)*EP29_HEIGHT;
public:
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
   static constexpr uint8_t COMMAND_DATA_ENTRY_MODE_SETTING = 0x11;
   static constexpr uint8_t COMMAND_SW_RESET = 0x12;
   static constexpr uint8_t COMMAND_TEMP_SET = 0x1A;
   static constexpr uint8_t COMMAND_MASTER_ACTIVATION = 0x20;
   static constexpr uint8_t COMMAND_DISPLAY_SETUP = 0x22;
   static constexpr uint8_t COMMAND_WRITE_RAM = 0x24;
   static constexpr uint8_t COMMAND_READ_ID = 0x2E;
   static constexpr uint8_t COMMAND_VCOM_REGISTER = 0x2C;
   static constexpr uint8_t COMMAND_WRITE_LUT = 0x32;
   static constexpr uint8_t COMMAND_DUMMY_LINE_PERIOD = 0x3A;
   static constexpr uint8_t COMMAND_SET_GATE_TIME = 0x3B;
   static constexpr uint8_t COMMAND_BORDER_WAVEFORM_CONTROL = 0x3C;
   static constexpr uint8_t COMMAND_SET_RAM_X = 0x44;
   static constexpr uint8_t COMMAND_SET_RAM_Y = 0x45;
   static constexpr uint8_t COMMAND_BOOSTER_SOFT_START_CONTROL = 0x0C;
public:
  //inits SPI Bus as well as display
   static ErrorType initBusAndDisplayDevice(gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, 
	   gpio_num_t cs, gpio_num_t resetPin, gpio_num_t busyPin, spi_host_device_t spiNum, SPIDevice *&sd );
   // inits device without initting SPI bus
   // sd = spidevice created on the spi_host_device
   static ErrorType initDisplayDevice(spi_host_device_t spiNum, gpio_num_t resetPin, gpio_num_t busyPin, 
         gpio_num_t cs, SPIDevice *&sd);
public:
	EPDisplay(ROTATION r, gpio_num_t bl, gpio_num_t reset, gpio_num_t busyPin, DISPLAY_TYPE dt);
   ErrorType init(SPIDevice *s, gpio_num_t data_cmd, const FontDef_t *defaultFont, uint8_t *fb);
	virtual ~EPDisplay();
	virtual bool drawPixel(int16_t x0, int16_t y0, const RGBColor &color);
	virtual void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	virtual void drawRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	virtual void fillScreen(const RGBColor &color);
	virtual void drawImage(int16_t x, int16_t y, const DCImage &dcImage);
	void setMemoryAccessControl();

	virtual void swap();
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
protected:
	void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
	bool writeCmd(uint8_t c);
	bool writeNData(const uint8_t *data, int nbytes);
	bool write16Data(const uint16_t &data);
	bool writeN(char dc, const uint8_t *data, int nbytes);
   bool readNData(uint8_t *data, int nbytes);
   bool isBusy(uint32_t timeoutMS);
   void setCursor(uint16_t Xstart, uint16_t Ystart);
private:
	RGBColor CurrentTextColor;
	RGBColor CurrentBGColor;
	gpio_num_t ResetPin;
   DISPLAY_TYPE DisplayType;
	gpio_num_t BackLight;
   uint8_t MemoryAccessControl;
	gpio_num_t BusyPin;
   uint8_t PixelFormat;
	SPIDevice *SPI;
   uint8_t *BackBuffer;
   gpio_num_t DataCmdPin;
   sCmdBuf *InitCmds;
};

}
