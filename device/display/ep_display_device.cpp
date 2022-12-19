#include "device/display/ep_display_device.h"
#include "epdisplay_device.h"
#include <esp_log.h>
#include "assert.h"
#include <string.h>
#include "frame_buffer.h"
#include "../../task.h"
#include "../../spibus.h"
#include "../../system.h"
#include "color.h"
#include "hal/gpio_types.h"
using namespace libesp;

static const char *LOGTAG="EPDisplay";

////////////////////////////////////////////////////////
// STATIC
ErrorType EPDisplay::initDisplay(gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, 
	int channel, gpio_num_t dataCmdPin, gpio_num_t resetPin, gpio_num_t busyPin, spi_host_device_t spiNum ) {

	ErrorType et;

	static const char *LOGTAG = "initDisplay";
	ESP_LOGI(LOGTAG,"Start initDisplay");
	spi_bus_config_t buscfg;
   buscfg.miso_io_num=miso;
   buscfg.mosi_io_num=mosi;
   buscfg.sclk_io_num=clk;
   buscfg.quadwp_io_num=-1;
   buscfg.quadhd_io_num=-1;
   buscfg.max_transfer_sz=10000;
   buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
   buscfg.intr_flags = 0;

	et = libesp::SPIBus::initializeBus(spiNum,buscfg,channel);
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"error initing BUS for touch %s", et.toString());
	} else {
		ESP_LOGI(LOGTAG,"SPIBus initiatlized for display");
    et = initDisplay(dataCmdPin, resetPin, busyPin);
	}
	return et;
}

//init if SPI bus is already initialized
ErrorType EPDisplay::initDisplay(gpio_num_t dataCmdPin, gpio_num_t resetPin, gpio_num_t busyPin) {
  ErrorType et;
  et = gpio_set_direction(dataCmdPin, GPIO_MODE_OUTPUT);
  if(et.ok()) {
    if(resetPin!=NOPIN) {
      et = gpio_set_direction(resetPin, GPIO_MODE_OUTPUT);
    }
    if(et.ok()) {
      et = gpio_set_direction(busyPin,GPIO_MODE_INPUT);
    }
  }
  return et;
}

////
EPDisplay::EPDisplay(uint16_t w, uint16_t h, EPDisplay::ROTATION r, gpio_num_t bl
    , gpio_num_t reset, gpio_num_t busyPin, EPDisplay::DISPLAY_TYPE dt) :
		DisplayDevice(w, h, r), CurrentTextColor(RGBColor::BLACK), CurrentBGColor(RGBColor::WHITE)
      , Reset(reset), DisplayType(dt), BackLight(NOPIN), MemoryAccessControl(0), BusyPin(busyPin) {

}

EPDisplay::~EPDisplay() {

}

void EPDisplay::swap() {
   return swap(false);
}

void EPDisplay::swap(bool block) {
	setMemoryAccessControl();
	getFrameBuffer()->swap(block);
}

void EPDisplay::drawImage(int16_t x, int16_t y, const DCImage &dcImage) {
	getFrameBuffer()->drawImage(x,y,dcImage);
}

bool EPDisplay::drawPixel(int16_t x0, int16_t y0, const RGBColor &color) {
	return getFrameBuffer()->drawPixel(x0, y0, color);
}

void EPDisplay::setBackLightOn(bool on) {
	if (BackLight != NOPIN)
		gpio_set_level(BackLight, on);
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
		getFrameBuffer()->writeCmd(EPDisplay::COMMAND_DATA_ENTRY);
		getFrameBuffer()->writeNData(&MemoryAccessControl, 1);
	}
}

struct sCmdBuf {
	uint8_t command;   // ST7735 command byte
	uint8_t delay;     // ms delay after
	uint8_t len;       // length of parameter data
	uint8_t data[32];  // parameter data
};


static constexpr const uint16_t EP29_WIDTH = 128;
static constexpr const uint16_t EP29_HEIGHT = 296;

DRAM_ATTR static const sCmdBuf ep29_init_cmds[]= {
	// SWRESET Software reset
	{ EPDisplay::COMMAND_SW_RESET, 100, 0, {} },
	{ EPDisplay::COMMAND_DRIVER_OUTPUT, 100, 3, {((EP29_HEIGHT-1)&0xFF), (((EP29_HEIGHT-1)>>8)&0xFF), 0x0} },
   { EPDisplay::COMMAND_DATA_ENTRY, 100, 1, {EPDisplay::X_COUNT_Y_INC_X_INC}},
   { EPDisplay::COMMAND_SET_RAM_X, 100, 2, { 0x00, 0x18 }},
   { EPDisplay::COMMAND_SET_RAM_Y, 100, 4, { 0x27, 0x1, 0x0, 0x0}},
   //set temperature to 25C
   { EPDisplay::COMMAND_TEMP_SET, 100, 2, {0x19, 0x0} },
   { EPDisplay::COMMAND_DISPLAY_SETUP, 100, 1, {0xb1}}, 
   { EPDisplay::COMMAND_WRITE_LUT, 100, 30 , { 0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}
};

void EPDisplay::reset() {

	if (Reset != NOPIN) {
		gpio_set_level(Reset, 1);
		vTaskDelay(10/portTICK_PERIOD_MS);
		gpio_set_level(Reset, 0);
		setBackLightOn(false);
		vTaskDelay(20/portTICK_PERIOD_MS);
		gpio_set_level(Reset, 1);
		setBackLightOn(true);
	}

  const sCmdBuf *cmd = nullptr;
  switch(getDisplayType()) {
    case EP2_9:
      cmd = ep29_init_cmds;
      break;
  }
	//for (const sCmdBuf *cmd = initializers; cmd->command; cmd++) {
	for (; cmd->command; cmd++) {
		getFrameBuffer()->writeCmd(cmd->command);
		if (cmd->len)
			getFrameBuffer()->writeNData(cmd->data, cmd->len);
		if (cmd->delay)
			vTaskDelay(cmd->delay/portTICK_PERIOD_MS);
	}
}

ErrorType EPDisplay::init(uint8_t pf, const FontDef_t *defaultFont, FrameBuf *fb) {  
	ErrorType et; 
	setFrameBuffer(fb);
	setFont(defaultFont);
	setBackLightOn(true);
	//ensure pixel format
   switch(getDisplayType()) {
      case EP2_9:
         PixelFormat = FORMAT_1_BIT;
         break;
   }

	reset();
	//ensure memory access control format
	setMemoryAccessControl();

	ESP_LOGI(LOGTAG,"init: fill screen BLACK");
	fillScreen(RGBColor::BLACK);
	swap();
   fillScreen(RBGColor::WHITE);
   swap();
	return et;
}

void EPDisplay::fillScreen(const RGBColor &color) {
	fillRec(0, 0, getFrameBuffer()->getBufferWidth(), getFrameBuffer()->getBufferHeight(), color);
}

// Draw a filled rectangle at the given coordinates with the given width, height, and color.
// Input: x     horizontal position of the top left corner of the rectangle, columns from the left edge
//        y     vertical position of the top left corner of the rectangle, rows from the top edge
//        w     horizontal width of the rectangle
//        h     vertical height of the rectangle
//        color appropriated packed color, which can be produced by PackColor::create()
// Output: none
void EPDisplay::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	//PackedColor pc = PackedColor::create(PixelFormat, color);

	if ((x >= getFrameBuffer()->getBufferWidth()) || (y >= getFrameBuffer()->getBufferHeight()))
		return;
	if ((x + w - 1) >= getFrameBuffer()->getBufferWidth())
		w = getFrameBuffer()->getBufferWidth() - x;
	if ((y + h - 1) >= getFrameBuffer()->getBufferHeight())
		h = getFrameBuffer()->getBufferHeight() - y;

	getFrameBuffer()->fillRec(x, y, w, h, color);
}

void EPDisplay::drawRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	drawHorizontalLine(x, y, w, color);
	drawVerticalLine(x, y, h, color);
	drawHorizontalLine(x, y + h >= getFrameBuffer()->getBufferHeight() ? getFrameBuffer()->getBufferHeight() - 1 : y + h, w, color);
	drawVerticalLine(x + w, y, h, color);
}

// Input: x         horizontal position of the top left corner of the character, columns from the left edge
//        y         vertical position of the top left corner of the character, rows from the top edge
//        c         character to be printed
//        textColor 16-bit color of the character
//        bgColor   16-bit color of the background
//        size      number of pixels per character pixel (e.g. size==2 prints each pixel of font as 2x2 square)
// Output: none
void EPDisplay::drawCharAtPosition(int16_t x, int16_t y, char c, const RGBColor &textColor, const RGBColor &bgColor,
		uint8_t size) {
	uint8_t line; // vertical column of pixels of character in font
	int32_t i, j;
	if ((x >= getFrameBuffer()->getBufferWidth()) || // Clip right
			(y >= getFrameBuffer()->getBufferHeight()) || // Clip bottom
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
		if ((currentX > getFrameBuffer()->getBufferWidth() && !lineWrap) || currentY > getFrameBuffer()->getBufferHeight()) {
			return pt - orig;
		} else if (currentX > getFrameBuffer()->getBufferWidth() && lineWrap) {
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
		if ((currentX > getFrameBuffer()->getBufferWidth() && !lineWrap) || currentY > getFrameBuffer()->getBufferHeight()) {
			return pt - orig;
		} else if (currentX > getFrameBuffer()->getBufferWidth() && lineWrap) {
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
	if ((x >= getFrameBuffer()->getBufferWidth()) || (y >= getFrameBuffer()->getBufferHeight()))
		return;
	if ((y + h - 1) >= getFrameBuffer()->getBufferWidth())
		h = getFrameBuffer()->getBufferWidth() - y;
	getFrameBuffer()->drawVerticalLine(x, y, h, color);
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
	if ((x >= getFrameBuffer()->getBufferWidth()) || (y >= getFrameBuffer()->getBufferHeight()))
		return;
	if ((x + w - 1) >= getFrameBuffer()->getBufferWidth())
		w = getFrameBuffer()->getBufferWidth() - x;

	getFrameBuffer()->drawHorizontalLine(x, y, w, color);
}

