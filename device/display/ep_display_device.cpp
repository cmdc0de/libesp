#include "ep_display_device.h"
#include <esp_log.h>
#include "assert.h"
#include <string.h>
#include "../../task.h"
#include "../../spibus.h"
#include "../../system.h"
#include "color.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "hal/spi_types.h"
#include "../../freertos.h"
#include "../../spidevice.h"

using namespace libesp;

static const char *LOGTAG="EPDisplay";


namespace libesp {
struct sCmdBuf {
	uint8_t command;   // ST7735 command byte
	uint8_t delay;     // ms delay after
	uint8_t len;       // length of parameter data
	uint8_t data[32];  // parameter data
};
}

DRAM_ATTR static const sCmdBuf ep29_init_cmds[]= {
	// SWRESET Software reset
	{ EPDisplay::COMMAND_SW_RESET, 200, 0, {} },
	{ EPDisplay::COMMAND_DRIVER_OUTPUT, 100, 3, {((EPDisplay::EP29_HEIGHT-1)&0xFF), (((EPDisplay::EP29_HEIGHT-1)>>8)&0xFF), 0x0} },
   { EPDisplay::COMMAND_BOOSTER_SOFT_START_CONTROL, 100, 3, {0xD7, 0xD6, 0x9D}},
   { EPDisplay::COMMAND_VCOM_REGISTER, 100, 1, {0xA8}},
   { EPDisplay::COMMAND_DUMMY_LINE_PERIOD, 100, 1, { 0x1A }},
   { EPDisplay::COMMAND_SET_GATE_TIME, 100, 1, {0x8}},
   { EPDisplay::COMMAND_BORDER_WAVEFORM_CONTROL, 100, 1, {0x3}},
   { EPDisplay::COMMAND_DATA_ENTRY_MODE_SETTING, 100, 1, {0x3}},
   { EPDisplay::COMMAND_DATA_ENTRY, 100, 1, {EPDisplay::X_COUNT_Y_INC_X_INC}},
   { EPDisplay::COMMAND_SET_RAM_X, 100, 2, { 0x00, 0x18 }},
   { EPDisplay::COMMAND_SET_RAM_Y, 100, 4, { 0x27, 0x1, 0x0, 0x0}},
   //set temperature to 25C
   { EPDisplay::COMMAND_TEMP_SET, 100, 2, {0x19, 0x0} },
   { EPDisplay::COMMAND_DISPLAY_SETUP, 100, 1, {0xb1}}, 
   { EPDisplay::COMMAND_WRITE_LUT, 100, 30 , { 0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
   { 0x00, 0x00, 0x00, {0x00}}
};

/*
   struct DisplaySpecs {
      uint16_t Width;
      uint16_t Height;
      uint32_t BackBufferSize;
      PIXEL_FORMAT PixelFormat;
      sCmdBuf *initCmds
      uint16_t NumCmds;
   };
*/

const EPDisplay::DisplaySpecs Specs[] = {
   {EPDisplay::EP29_WIDTH, EPDisplay::EP29_HEIGHT, 
      (EPDisplay::EP29_WIDTH /8)*EPDisplay::EP29_HEIGHT, EPDisplay::PIXEL_FORMAT::FORMAT_1_BIT
      , &ep29_init_cmds[0], sizeof(ep29_init_cmds)/sizeof(ep29_init_cmds[0])}
};

const EPDisplay::DisplaySpecs &EPDisplay::getSpecs(const EPDisplay::DISPLAY_TYPE &dt) {
   return Specs[dt];
}

struct SPIUserData {
   SPIUserData(uint8_t state, gpio_num_t p) : DCState(state), DCPin(p) {}
  uint8_t DCState;
  gpio_num_t DCPin;
};

void spi_pre_epdcb(spi_transaction_t *t) {
	SPIUserData* ud=reinterpret_cast<SPIUserData*>(t->user);
	gpio_set_level(ud->DCPin, ud->DCState);
}

////////////////////////////////////////////////////////
// STATIC
ErrorType EPDisplay::initBusAndDisplayDevice(gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, 
	gpio_num_t cs, gpio_num_t resetPin, gpio_num_t busyPin, spi_host_device_t spiNum, SPIDevice *&sd ) {

	ErrorType et;

	static const char *LOGTAG = "EPD initDisplay";
	ESP_LOGI(LOGTAG,"Start EPD initDisplay");
	spi_bus_config_t buscfg;
   memset(&buscfg, 0, sizeof(buscfg));
   buscfg.miso_io_num=miso;
   buscfg.mosi_io_num=mosi;
   buscfg.sclk_io_num=clk;
   buscfg.quadwp_io_num=-1;
   buscfg.quadhd_io_num=-1;
   buscfg.max_transfer_sz=10000;
   buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
   buscfg.intr_flags = 0;

   //channel must be 0 since esp32 does not support DMA in half duplex
	et = libesp::SPIBus::initializeBus(spiNum,buscfg,0);
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"error initing BUS for EP %s", et.toString());
	} else {
		ESP_LOGI(LOGTAG,"SPIBus initiatlized for display");
      et = initDisplayDevice(spiNum, resetPin, busyPin, cs, sd);
	}
	return et;
}

//init if SPI bus is already initialized
ErrorType EPDisplay::initDisplayDevice(spi_host_device_t spiNum, gpio_num_t resetPin, gpio_num_t busyPin,
      gpio_num_t cs, SPIDevice *&sd) {
   ErrorType et;
   if(resetPin!=NOPIN) {
      et = gpio_set_direction(resetPin, GPIO_MODE_OUTPUT);
   }
   if(et.ok() && busyPin!=NOPIN) {
      ESP_LOGI(LOGTAG,"busyPin set to input");
      et = gpio_set_direction(busyPin,GPIO_MODE_INPUT);
   }
	ESP_LOGI(LOGTAG,"start createInitDevice");
	spi_device_interface_config_t devcfg;
	memset(&devcfg,0,sizeof(devcfg));
	//this could be calculated!!!
	devcfg.clock_speed_hz=SPI_MASTER_FREQ_8M; //1*1000*1000;         
	devcfg.mode=0;          //SPI mode 0
	devcfg.spics_io_num=cs; //CS pin
	//TODO This should be calculated based on SPIBus buffer size and number of
	devcfg.queue_size=2; //# of transactions at a time
	devcfg.duty_cycle_pos = 0;
	devcfg.cs_ena_pretrans = 0;
	devcfg.cs_ena_posttrans = 0;
	devcfg.input_delay_ns = 0;
	devcfg.flags = SPI_DEVICE_HALFDUPLEX|SPI_DEVICE_3WIRE|SPI_DEVICE_NO_DUMMY;
	devcfg.pre_cb = spi_pre_epdcb;
	devcfg.post_cb = nullptr;

	sd = SPIBus::get(spiNum)->createMasterDevice(devcfg, nullptr);
	if(!sd) {
		ESP_LOGE(LOGTAG,"failed createInitDevice: EPD");
		return ErrorType();
	}
   return et;
}

////
EPDisplay::EPDisplay(EPDisplay::ROTATION r, gpio_num_t bl, gpio_num_t resetP, gpio_num_t busyPin
      , EPDisplay::DISPLAY_TYPE dt) : DisplayDevice(getSpecs(dt).Width, getSpecs(dt).Height, r)
      , CurrentTextColor(RGBColor::BLACK), CurrentBGColor(RGBColor::WHITE), ResetPin(resetP), DisplayType(dt)
      , BackLight(NOPIN), MemoryAccessControl(0), BusyPin(busyPin), PixelFormat(FORMAT_1_BIT), SPI(nullptr)
      , BackBuffer(nullptr), InitCmds(nullptr)  {

}

EPDisplay::~EPDisplay() {

}

bool EPDisplay::isBusy(uint32_t timeoutMS) {
   ESP_LOGD(LOGTAG, "e-Paper busy");
   uint8_t busy;
   uint32_t startTime = FreeRTOS::getTimeSinceStart();
   do {
      writeCmd(0x71); //what is this?
      busy = gpio_get_level(BusyPin);
      //busy =!(busy & 0x01);
      vTaskDelay(10/portTICK_PERIOD_MS); 
      if((FreeRTOS::getTimeSinceStart()-startTime)>timeoutMS) {
         ESP_LOGI(LOGTAG,"timeout waiting for EPD busy to release");
         return false;
      }
   } while(busy);
   vTaskDelay(10/portTICK_PERIOD_MS); 
   ESP_LOGD(LOGTAG, "e-Paper Release");
   return true;
}

void EPDisplay::setCursor(uint16_t Xstart, uint16_t Ystart) {
   writeCmd(0x4E); // SET_RAM_X_ADDRESS_COUNTER
   uint8_t data = (Xstart >> 3) & 0xFF;
   writeNData(&data, 1);

   writeCmd(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
   data = Ystart & 0xFF;
   writeNData(&data, 1);
   data = (Ystart >> 8) & 0xFF;
   writeNData(&data, 1);
}

void EPDisplay::swap() {
	setMemoryAccessControl();
   uint8_t Width, Height;
   Width = (getScreenWidth() % 8 == 0)? (getScreenWidth() / 8 ): (getScreenWidth() / 8 + 1);
   Height = getScreenHeight();

   setAddrWindow(0, 0, getScreenWidth(), getScreenHeight());
   for (uint16_t j = 0; j < Height; j++) {
      setCursor(0, j);
      writeCmd(COMMAND_WRITE_RAM);
      uint16_t Addr = j * Width;
      writeNData(&BackBuffer[Addr],Width);
   }

   //turn on display    
   writeCmd(COMMAND_DISPLAY_SETUP);		 //DISPLAY REFRESH
   uint8_t data = 0xC4;
   writeNData(&data,1);
   writeCmd(COMMAND_MASTER_ACTIVATION);
   data = 0xFF;
   writeNData(&data,1);
   //!!!The delay here is necessary, 200uS at least!!!
   vTaskDelay(10/portTICK_PERIOD_MS); 
   isBusy(20000);
}

void EPDisplay::drawImage(int16_t x, int16_t y, const DCImage &dcImage) {
	drawImage(x,y,dcImage);
}

bool EPDisplay::drawPixel(int16_t x0, int16_t y0, const RGBColor &color) {
	uint32_t index = (x0/8)+((getSpecs(getDisplayType()).Width/8)*y0);
   uint32_t bitOffset = x0%8;
   if(color==RGBColor::BLACK) {
      BackBuffer[index] = (BackBuffer[index] & ~(1<<bitOffset));
   } else {
      BackBuffer[index] |= (1<<bitOffset);
   }
   return true;
}

void EPDisplay::setBackLightOn(bool on) {
	if (BackLight != NOPIN)
		gpio_set_level(BackLight, on);
}

void EPDisplay::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
   writeCmd(COMMAND_SET_RAM_X); // SET_RAM_X_ADDRESS_START_END_POSITION
   uint8_t data = (x0 >> 3) & 0xFF;
   writeNData(&data,1);
   data = ((x1 >> 3) & 0xFF);
   writeNData(&data,1);

   writeCmd(COMMAND_SET_RAM_Y); // SET_RAM_Y_ADDRESS_START_END_POSITION
   uint8_t d[2];
   d[0] = y0 & 0xFF;
   d[1] = (y0 >> 8) & 0xFF;
   writeNData(&d[0],sizeof(d));
   d[0] = y1 & 0xFF;
   d[1] = ((y1 >> 8) & 0xFF);
   writeNData(&d[0],sizeof(d));
}

bool EPDisplay::writeCmd(uint8_t c) {
	return writeN(0, &c, sizeof(c));
}

bool EPDisplay::writeNData(const uint8_t *data, int nbytes) {
	return writeN(1, data, nbytes);
}

bool EPDisplay::write16Data(const uint16_t &data) {
	uint8_t buf[2];
	buf[0] = data >> 8;
	buf[1] = data & 0xFF;
	return writeN(1, &buf[0], sizeof(buf));
}

bool EPDisplay::readNData(uint8_t *data, int nbytes) {
   SPIUserData ud(1,DataCmdPin);
   ErrorType et;// = SPI->receive(data,nbytes,&ud);
   return et.ok();
}

bool EPDisplay::writeN(char dc, const uint8_t *data, int nbytes) {
   SPIUserData ud(dc,DataCmdPin);
	ErrorType et = SPI->send(data,nbytes,&ud);
	return et.ok();
}

void EPDisplay::setMemoryAccessControl() {
	uint8_t macctl  = 0;
	switch(getRotation()) {
		case EPDisplay::LANDSCAPE_TOP_LEFT:
      if(getDisplayType()==EP2_9)
			  macctl =  X_COUNT_Y_INC_X_INC;
			break;
		case EPDisplay::PORTAIT_TOP_LEFT:
		default:
			break;
	}

	if (macctl != MemoryAccessControl) {
		MemoryAccessControl = macctl;
		writeCmd(EPDisplay::COMMAND_DATA_ENTRY);
		writeNData(&MemoryAccessControl, 1);
	}
}





void EPDisplay::reset() {
   gpio_set_level(ResetPin, 1);
   vTaskDelay(20/portTICK_PERIOD_MS); 
   gpio_set_level(ResetPin, 0);
   vTaskDelay(2/portTICK_PERIOD_MS); 
   gpio_set_level(ResetPin, 1);
   vTaskDelay(20/portTICK_PERIOD_MS); 
   gpio_set_level(ResetPin, 0);
   vTaskDelay(2/portTICK_PERIOD_MS); 
   gpio_set_level(ResetPin, 1);
   vTaskDelay(20/portTICK_PERIOD_MS); 
   gpio_set_level(ResetPin, 0);
   vTaskDelay(2/portTICK_PERIOD_MS); 
   gpio_set_level(ResetPin, 1);
   vTaskDelay(20/portTICK_PERIOD_MS); 
}

ErrorType EPDisplay::init(SPIDevice *s, gpio_num_t data_cmd, const FontDef_t *defaultFont, uint8_t *fb) {  
	ErrorType et; 
   SPI = s;
	DataCmdPin = data_cmd;
   et = gpio_set_direction(DataCmdPin, GPIO_MODE_OUTPUT);
	setFont(defaultFont);
	setBackLightOn(true);
   BackBuffer = fb;
   const sCmdBuf *initCmds = nullptr;
	//ensure pixel format
   switch(getDisplayType()) {
      case EP2_9:
         PixelFormat = FORMAT_1_BIT;
         initCmds = &ep29_init_cmds[0];
         break;
      default:
         PixelFormat = FORMAT_1_BIT;
         break;
   }
	reset();

   int16_t i = 0;
   ESP_LOGI(LOGTAG,"Sending initial commands");
   while(initCmds[i].command!=0x00) {
      writeCmd(initCmds[i].command);
      vTaskDelay(initCmds[i].delay/portTICK_PERIOD_MS);
      writeNData(&initCmds[i].data[0], initCmds[i].len);
      vTaskDelay(100/portTICK_PERIOD_MS);
      ++i;
   }
	uint8_t buf[10] = {0};
   writeCmd(COMMAND_READ_ID); //read id
   readNData(&buf[0],sizeof(buf));
	ESP_LOGI(LOGTAG,"DISPLAY ID");
	ESP_LOG_BUFFER_HEX(LOGTAG,&buf[0],sizeof(buf));

	//ensure memory access control format
	setMemoryAccessControl();

	ESP_LOGI(LOGTAG,"init: fill screen BLACK");
	fillScreen(RGBColor::BLACK);
	swap();
   fillScreen(RGBColor::WHITE);
   swap();
	return et;
}

void EPDisplay::fillScreen(const RGBColor &color) {
	fillRec(0, 0, getScreenWidth(), getScreenHeight(), color);
}

// Draw a filled rectangle at the given coordinates with the given width, height, and color.
// Input: x     horizontal position of the top left corner of the rectangle, columns from the left edge
//        y     vertical position of the top left corner of the rectangle, rows from the top edge
//        w     horizontal width of the rectangle
//        h     vertical height of the rectangle
//        color appropriated packed color, which can be produced by PackColor::create()
// Output: none
void EPDisplay::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	if ((x >= getScreenWidth()) || (y >= getScreenHeight()))
		return;
	if ((x + w - 1) >= getScreenWidth())
		w = getScreenWidth() - x;
	if ((y + h - 1) >= getScreenHeight())
		h = getScreenHeight() - y;

   for(int16_t i = y;i<(y+h);++i) {
      for(int16_t k = x;k<(x+w);++k) {
         drawPixel(k, i, color);
      }
   }
}

void EPDisplay::drawRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	drawHorizontalLine(x, y, w, color);
	drawVerticalLine(x, y, h, color);
	drawHorizontalLine(x, y + h >= getScreenHeight() ? getScreenHeight() - 1 : y + h, w, color);
	drawVerticalLine(x + w, y, h, color);
}

// Input: x         horizontal position of the top left corner of the character, columns from the left edge
//        y         vertical position of the top left corner of the character, rows from the top edge
//        c         character to be printed
//        textColor 16-bit color of the character
//        bgColor   16-bit color of the background
//        size      number of pixels per character pixel (e.g. size==2 prints each pixel of font as 2x2 square)
// Output: none
void EPDisplay::drawCharAtPosition(int16_t x, int16_t y, char c, const RGBColor &textColor, const RGBColor &bgColor, uint8_t size) {
	uint8_t line; // vertical column of pixels of character in font
	int32_t i, j;
	if ((x >= getScreenWidth()) || // Clip right
			(y >= getScreenHeight()) || // Clip bottom
			((x + 5 * size - 1) < 0) || // Clip left
			((y + 8 * size - 1) < 0))   // Clip top
		return;

	for (i = 0; i < getFont()->FontWidth; i++) {
		if (i == getFont()->FontWidth - 1)
			line = 0x0;
		else
			line = getFontData()[(c * getFont()->CharBytes) + i];
		for (j = 0; j < 8; j++) {
			if (line & 0x1) {
				if (size == 1) // default size
					drawPixel(x + i, y + j, textColor);
				else {  // big size
					fillRec(x + (i * size), y + (j * size), size, size, textColor);
				}
			} else if (bgColor != textColor) {
				if (size == 1) // default size
					drawPixel(x + i, y + j, bgColor);
				else {  // big size
					fillRec(x + i * size, y + j * size, size, size, bgColor);
				}
			}
			line >>= 1;
		}
	}
}

void EPDisplay::setTextColor(const RGBColor &t) {
	CurrentTextColor = t;
}

void EPDisplay::setBackgroundColor(const RGBColor &t) {
	CurrentBGColor = t;
}

const RGBColor &EPDisplay::getTextColor() {
	return CurrentTextColor;
}

const RGBColor &EPDisplay::getBackgroundColor() {
	return CurrentBGColor;
}

uint32_t EPDisplay::drawStringOnLine(uint8_t line, const char *msg) {
	return drawString(0, getFont()->FontHeight * line, msg, RGBColor::WHITE, RGBColor::BLACK, 1, true);
}

uint32_t EPDisplay::drawString(uint16_t x, uint16_t y, const char *pt) {
	return drawString(x, y, pt, CurrentTextColor);
}

uint32_t EPDisplay::drawString(uint16_t x, uint16_t y, const char *pt, const RGBColor &textColor) {
	return drawString(x, y, pt, textColor, CurrentBGColor, 1, false);
}

uint32_t EPDisplay::drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor,
		const RGBColor &backGroundColor, uint8_t size, bool lineWrap) {
	uint16_t currentX = xPos;
	uint16_t currentY = yPos;
	const char *orig = pt;

	while (*pt) {
		if ((currentX > getScreenWidth() && !lineWrap) || currentY > getScreenHeight()) {
			return pt - orig;
		} else if (currentX > getScreenWidth() && lineWrap) {
			currentX = 0;
			currentY += getFont()->FontHeight * size;
			drawCharAtPosition(currentX, currentY, *pt, textColor, backGroundColor, size);
			currentX += getFont()->FontWidth;
		} else if (*pt == '\n' || *pt == '\r') {
			currentY += getFont()->FontHeight * size;
			currentX = 0;
		} else {
			drawCharAtPosition(currentX, currentY, *pt, textColor, backGroundColor, size);
			currentX += getFont()->FontWidth * size;
		}
		pt++;
	}
	return (pt - orig);  // number of characters printed
}

uint32_t EPDisplay::drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor,
		const RGBColor &backGroundColor, uint8_t size, bool lineWrap, uint8_t charsToRender) {
	uint16_t currentX = xPos;
	uint16_t currentY = yPos;
	const char *orig = pt;

	while (charsToRender-- && *pt) {
		if ((currentX > getScreenWidth() && !lineWrap) || currentY > getScreenHeight()) {
			return pt - orig;
		} else if (currentX > getScreenWidth() && lineWrap) {
			currentX = 0;
			currentY += getFont()->FontHeight * size;
			drawCharAtPosition(currentX, currentY, *pt, textColor, backGroundColor, size);
			currentX += getFont()->FontWidth;
		} else if (*pt == '\n' || *pt == '\r') {
			currentY += getFont()->FontHeight * size;
			currentX = 0;
		} else {
			drawCharAtPosition(currentX, currentY, *pt, textColor, backGroundColor, size);
			currentX += getFont()->FontWidth * size;
		}
		pt++;
	}
	return (pt - orig);  // number of characters printed
}

void EPDisplay::drawVerticalLine(int16_t x, int16_t y, int16_t h) {
	drawVerticalLine(x, y, h, CurrentTextColor);
}

// Draw a vertical line at the given coordinates with the given height and color.
// A vertical line is parallel to the longer side of the rectangular display
// Input: x     horizontal position of the start of the line, columns from the left edge
//        y     vertical position of the start of the line, rows from the top edge
//        h     vertical height of the line
//		color	RGB color of line
void EPDisplay::drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color) {
	// safety
	if ((x >= getScreenWidth()) || (y >= getScreenHeight()))
		return;
	if ((y + h - 1) >= getScreenWidth())
		h = getScreenWidth() - y;
	
   for (int i = y; i < (h + y); ++i) {
		drawPixel(x, y, color);
	}
}

void EPDisplay::drawHorizontalLine(int16_t x, int16_t y, int16_t w) {
	return drawHorizontalLine(x, y, w, CurrentTextColor);
}

// Draw a horizontal line at the given coordinates with the given width and color.
// Input: x     horizontal position of the start of the line, columns from the left edge
//        y     vertical position of the start of the line, rows from the top edge
//        w     horizontal width of the line
//		Color is the RGBColor
void EPDisplay::drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color) {
	//safey
	if ((x >= getScreenWidth()) || (y >= getScreenHeight()))
		return;
	if ((x + w - 1) >= getScreenWidth())
		w = getScreenWidth() - x;

	uint32_t offset = y*getScreenWidth();
	for(int i=x;i<(x+w);++i) {
		drawPixel(offset+i, y, color);
	}
}

