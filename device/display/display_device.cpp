#include "display_device.h"
#include <esp_log.h>
#include "assert.h"
#include <string.h>
#include "frame_buffer.h"
#include "../../task.h"
#include "../../spibus.h"
#include "../../system.h"
#include "color.h"
using namespace libesp;

static const char *LOGTAG="Display";

/////////////////////////////////////////////////////////////////////
// Generic base display device
DisplayDevice::DisplayDevice(uint16_t w, uint16_t h,DisplayDevice::ROTATION r) : ScreenWidth(w), ScreenHeight(h), Rotation(r), RefreshTopToBot(0), FB(0), CurrentFont(0) {

}

DisplayDevice::~DisplayDevice() {

}

uint16_t DisplayDevice::getScreenWidth() const {
	return ScreenWidth;
}
uint16_t DisplayDevice::getScreenHeight() const {
	return ScreenHeight;
}
DisplayDevice::ROTATION DisplayDevice::getRotation() const {
	return Rotation == 0 ? PORTAIT_TOP_LEFT : LANDSCAPE_TOP_LEFT;
}

void DisplayDevice::setRotation(ROTATION r, bool swapHeightWidth) {
	Rotation = r;
	if(swapHeightWidth) {
		uint16_t tmp = ScreenHeight;
		ScreenHeight = ScreenWidth;
		ScreenWidth = tmp;
	}
}

bool DisplayDevice::isTopToBotRefresh() {
	return RefreshTopToBot;
}

void DisplayDevice::setTopToBotRefresh(bool b) {
	RefreshTopToBot = b;
}

const uint8_t *DisplayDevice::getFontData() {
	return CurrentFont->data;
}

void DisplayDevice::setFont(const FontDef_t *font) {
	CurrentFont = font;
}

////////////////////////////////////////////////////////
// STATIC
ErrorType DisplayILI9341::initDisplay(gpio_num_t miso,gpio_num_t mosi, gpio_num_t clk, 
	int channel, gpio_num_t dataCmdPin, gpio_num_t resetPin, gpio_num_t backlightPin,
	spi_host_device_t spiNum ) {

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
		et = gpio_set_direction(dataCmdPin, GPIO_MODE_OUTPUT);
		if(et.ok()) {
			if(resetPin!=NOPIN) {
				gpio_set_direction(resetPin, GPIO_MODE_OUTPUT);
			}
			if(backlightPin!=NOPIN) {
				gpio_set_direction(backlightPin, GPIO_MODE_OUTPUT);
			}
		}
	}
	return et;
}

////
DisplayILI9341::DisplayILI9341(uint16_t w, uint16_t h, DisplayILI9341::ROTATION r, gpio_num_t bl, gpio_num_t reset) :
		DisplayDevice(w, h, r), CurrentTextColor(RGBColor::WHITE), CurrentBGColor(
				RGBColor::BLACK), BackLight(bl), Reset(reset), 	MemoryAccessControl(1) /*1 is not valid*/, PixelFormat(0) {

}

DisplayILI9341::~DisplayILI9341() {

}

struct sCmdBuf {
	uint8_t command;   // ST7735 command byte
	uint8_t delay;     // ms delay after
	uint8_t len;       // length of parameter data
	uint8_t data[16];  // parameter data
};

DRAM_ATTR static const sCmdBuf ili_init_cmds[]= {
	// SWRESET Software reset
	{ DisplayILI9341::SWRESET, 150, 0, 0 },
	// SLPOUT Leave sleep mode
	{ DisplayILI9341::SLEEP_OUT, 150, 0, 0 },
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    {0xCF, 100, 3, {0x00, 0x83, 0X30}},
    /* Power on sequence control, cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2 DDVDH_ENH=1 */
    {0xED, 100, 4, {0x64, 0x03, 0X12, 0X81}},
    /* Driver timing control A, non-overlap=default +1 EQ=default - 1, CR=default
     * pre-charge=default - 1 */
    {0xE8, 100, 3, {0x85, 0x01, 0x79}},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, 100, 5, {0x39, 0x2C, 0x00, 0x34, 0x02}},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, 100, 1, {0x20}},
    /* Driver timing control, all=0 unit */
    {0xEA, 100, 2, {0x00, 0x00}},
    /* Power control 1, GVDD=4.75V */
    {0xC0, 100, 1, {0x26}},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, 100, 1, {0x11}},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, 100, 2, {0x35, 0x3E}},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, 100, 1, {0xBE}},
    /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    //{0x36, 100, 1, {0x28}},
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, 100, 1, {0x55}},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, 100, 2, {0x00, 0x1B}},
    /* Enable 3G, disabled */
    {0xF2, 100, 1, {0x08}},
    /* Gamma set, curve 1 */
    {0x26, 100, 1, {0x01}},
    /* Positive gamma correction */
    {0xE0, 100, 15, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}},
    /* Negative gamma correction */
    {0XE1, 100, 15, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}},
    /* Column address set, SC=0, EC=0xEF */
    {0x2A, 100, 4, {0x00, 0x00, 0x00, 0xEF}},
    /* Page address set, SP=0, EP=0x013F */
    {0x2B, 100, 4, {0x00, 0x00, 0x01, 0x3f}},
    /* Memory write */
    {0x2C,   0, 0, {0}},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, 100, 1, {0x07}},
    /* Display function control */
    {0xB6, 100, 4, {0x0A, 0x82, 0x27, 0x00}},
    /* Sleep out */
    {0x11, 100, 0, {0}},
    /* Display on */
    {0x29, 100, 0, {0}},
	// NORON Normal on
	{ DisplayILI9341::NORMAL_DISPLAY_MODE_ON, 10, 0, 0 },
    {0, 0, 0, 0},
};
static const struct sCmdBuf initializers[] = {
		// SWRESET Software reset
		{ DisplayILI9341::SWRESET, 150, 0, 0 },
		// SLPOUT Leave sleep mode
		{ DisplayILI9341::SLEEP_OUT, 150, 0, 0 },
		// FRMCTR1, FRMCTR2 Frame Rate configuration -- Normal mode, idle
		// frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
		{ DisplayILI9341::FRAME_RATE_CONTROL_FULL_COLOR, 0, 3, { 0x01, 0x2C, 0x2B } }, {
				DisplayILI9341::FRAME_RATE_CONTROL_IDLE_COLOR, 0, 3, { 0x01, 0x2C, 0x2B } },
		// FRMCTR3 Frame Rate configuration -- partial mode
		{ DisplayILI9341::FRAME_RATE_CONTROL_PARTIAL_FULL_COLOR, 0, 6, { 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D } },
		// INVCTR Display inversion (no inversion)
		{ DisplayILI9341::DISPLAY_INVERSION_CONTROL, 0, 1, { 0x07 } },
		// PWCTR1 Power control -4.6V, Auto mode
		{ DisplayILI9341::POWER_CONTROL_1, 0, 3, { 0xA2, 0x02, 0x84 } },
		// PWCTR2 Power control VGH25 2.4C, VGSEL -10, VGH = 3 * AVDD
		{ DisplayILI9341::POWER_CONTROL_2, 0, 1, { 0xC5 } },
		// PWCTR3 Power control, opamp current smal, boost frequency
		{ DisplayILI9341::POWER_CONTROL_3, 0, 2, { 0x0A, 0x00 } },
		// PWCTR4 Power control, BLK/2, opamp current small and medium low
		{ DisplayILI9341::POWER_CONTROL_4, 0, 2, { 0x8A, 0x2A } },
		// PWRCTR5, VMCTR1 Power control
		{ DisplayILI9341::POWER_CONTROL_5, 0, 2, { 0x8A, 0xEE } }, { 0xC5, 0, 1, { 0x0E } },
		// INVOFF Don't invert display
		{ DisplayILI9341::DISPLAY_INVERSION_OFF, 0, 0, 0 },
		// Memory access directions. row address/col address, bottom to top refesh (10.1.27)
		//{ DisplayILI9341::MEMORY_DATA_ACCESS_CONTROL, 0, 1, { DisplayILI9341::MADCTL_VERTICAL_REFRESH_ORDER_BOT_TOP } },
		// Color mode 18 bit (10.1.30
		//011 12 bit/pixel, 101 16 bit/pixel, 110 18 bit/pixel, 111 not used
		{ DisplayILI9341::INTERFACE_PIXEL_FORMAT, 0, 1, { 0b101 } },
		// Column address set 0..127
		//{ DisplayILI9341::COLUMN_ADDRESS_SET, 0, 4, { 0x00, 0x00, 0x00, 0x7F } },
		// set 0 ..240
		{ DisplayILI9341::COLUMN_ADDRESS_SET, 0, 4, { 0x00, 0x00, 0x00, 0xEF } },
		// Row address set 0..159
		//{ DisplayILI9341::ROW_ADDRESS_SET, 0, 4, { 0x00, 0x00, 0x00, 0x9F } },
		//0..320
		{ DisplayILI9341::ROW_ADDRESS_SET, 0, 4, { 0x00, 0x00,  0x01, 0x3f } },
		// GMCTRP1 Gamma correction
		{ 0xE0, 0, 16,
				{ 0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10 } },
		// GMCTRP2 Gamma Polarity correction
		{ 0xE1, 0, 16,
				{ 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10 } },
		// DISPON Display on
		{ DisplayILI9341::DISPLAY_ON, 100, 0, 0 },
		// NORON Normal on
		{ DisplayILI9341::NORMAL_DISPLAY_MODE_ON, 10, 0, 0 },
		// End
		{ 0, 0, 0, 0 } };

void DisplayILI9341::swap() {
	setMemoryAccessControl();
	getFrameBuffer()->swap();
}

void DisplayILI9341::drawImage(int16_t x, int16_t y, const DCImage &dcImage) {
	getFrameBuffer()->drawImage(x,y,dcImage);
}

bool DisplayILI9341::drawPixel(int16_t x0, int16_t y0, const RGBColor &color) {
	return getFrameBuffer()->drawPixel(x0, y0, color);
}

void DisplayILI9341::setBackLightOn(bool on) {

	if (BackLight != NOPIN)
		gpio_set_level(BackLight, !on);

	//FIX ME
	//if (on)
		//HAL_GPIO_WritePin(HardwareConfig::getBackLit().Port, HardwareConfig::getBackLit().Pin, GPIO_PIN_SET);
	//else
		//HAL_GPIO_WritePin(HardwareConfig::getBackLit().Port, HardwareConfig::getBackLit().Pin, GPIO_PIN_RESET);
}


void DisplayILI9341::setMemoryAccessControl() {
	uint8_t macctl  = 0;
	switch(getRotation()) {
		case DisplayILI9341::LANDSCAPE_TOP_LEFT:
			//macctl = MADCTL_MY|MADCTL_MV|MADCTL_MX;
			macctl = MADCTL_MY|MADCTL_MV|MADCTL_MX|MADCTL_BGR;
			//macctl = 0b11110000;
			break;
		case DisplayILI9341::PORTAIT_TOP_LEFT:
		default:
			break;
	}
	if(!isTopToBotRefresh()) {
		macctl |= DisplayILI9341::MADCTL_VERTICAL_REFRESH_ORDER_BOT_TOP;
	}

	if (macctl != MemoryAccessControl) {
		MemoryAccessControl = macctl;
		getFrameBuffer()->writeCmd(DisplayILI9341::MEMORY_DATA_ACCESS_CONTROL);
		getFrameBuffer()->writeNData(&MemoryAccessControl, 1);
	}
}

void DisplayILI9341::reset() {

	if (Reset != NOPIN)
	{
		gpio_set_level(Reset, 1);
		vTaskDelay(10/portTICK_PERIOD_MS);
		gpio_set_level(Reset, 0);
		setBackLightOn(false);
		vTaskDelay(20/portTICK_PERIOD_MS);
		gpio_set_level(Reset, 1);
		setBackLightOn(true);
	}

	//for (const sCmdBuf *cmd = initializers; cmd->command; cmd++) {
	for (const sCmdBuf *cmd = ili_init_cmds; cmd->command; cmd++) {
		getFrameBuffer()->writeCmd(cmd->command);
		if (cmd->len)
			getFrameBuffer()->writeNData(cmd->data, cmd->len);
		if (cmd->delay)
			vTaskDelay(cmd->delay/portTICK_PERIOD_MS);
	}
}

void DisplayILI9341::setPixelFormat(uint8_t pf) {
	if (PixelFormat != pf) {
		PixelFormat = pf;
		getFrameBuffer()->writeCmd(DisplayILI9341::INTERFACE_PIXEL_FORMAT);
		getFrameBuffer()->writeNData(&pf, 1);
		switch(pf) {
		case FORMAT_12_BIT:
			getFrameBuffer()->setPixelFormat(libesp::PackedColor::PIXEL_FORMAT_12_BIT);
			break;
		case FORMAT_16_BIT:
			getFrameBuffer()->setPixelFormat(libesp::PackedColor::PIXEL_FORMAT_16_BIT);
			break;
		case FORMAT_18_BIT:
			getFrameBuffer()->setPixelFormat(libesp::PackedColor::PIXEL_FORMAT_18_BIT);
			break;
		}
	}
}

ErrorType DisplayILI9341::init(uint8_t pf, const FontDef_t *defaultFont, FrameBuf *fb) {  
	ErrorType et; 
	setFrameBuffer(fb);
	setFont(defaultFont);
	setBackLightOn(true);
	//ensure pixel format
	setPixelFormat(pf);

	reset();
	//ensure memory access control format
	setMemoryAccessControl();

	ESP_LOGI(LOGTAG,"init: fill screen black");
	fillScreen(RGBColor::BLACK);
	swap();
	ESP_LOGI(LOGTAG,"init: fill screen black swap");
	return et;
}

void DisplayILI9341::fillScreen(const RGBColor &color) {
	fillRec(0, 0, getFrameBuffer()->getBufferWidth()-1, getFrameBuffer()->getBufferHeight()-1, color);
}

// Draw a filled rectangle at the given coordinates with the given width, height, and color.
// Input: x     horizontal position of the top left corner of the rectangle, columns from the left edge
//        y     vertical position of the top left corner of the rectangle, rows from the top edge
//        w     horizontal width of the rectangle
//        h     vertical height of the rectangle
//        color appropriated packed color, which can be produced by PackColor::create()
// Output: none
void DisplayILI9341::fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
	//PackedColor pc = PackedColor::create(PixelFormat, color);

	if ((x >= getFrameBuffer()->getBufferWidth()) || (y >= getFrameBuffer()->getBufferHeight()))
		return;
	if ((x + w - 1) >= getFrameBuffer()->getBufferWidth())
		w = getFrameBuffer()->getBufferWidth() - x;
	if ((y + h - 1) >= getFrameBuffer()->getBufferHeight())
		h = getFrameBuffer()->getBufferHeight() - y;

	getFrameBuffer()->fillRec(x, y, w, h, color);
}

void DisplayILI9341::drawRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
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
void DisplayILI9341::drawCharAtPosition(int16_t x, int16_t y, char c, const RGBColor &textColor, const RGBColor &bgColor,
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

void DisplayILI9341::setTextColor(const RGBColor &t) {
	CurrentTextColor = t;
}

void DisplayILI9341::setBackgroundColor(const RGBColor &t) {
	CurrentBGColor = t;
}

const RGBColor &DisplayILI9341::getTextColor() {
	return CurrentTextColor;
}

const RGBColor &DisplayILI9341::getBackgroundColor() {
	return CurrentBGColor;
}

uint32_t DisplayILI9341::drawStringOnLine(uint8_t line, const char *msg) {
	return drawString(0, getFont()->FontHeight * line, msg, RGBColor::WHITE, RGBColor::BLACK, 1, true);
}

uint32_t DisplayILI9341::drawString(uint16_t x, uint16_t y, const char *pt) {
	return drawString(x, y, pt, CurrentTextColor);
}

uint32_t DisplayILI9341::drawString(uint16_t x, uint16_t y, const char *pt, const RGBColor &textColor) {
	return drawString(x, y, pt, textColor, CurrentBGColor, 1, false);
}

uint32_t DisplayILI9341::drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor,
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

uint32_t DisplayILI9341::drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor,
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

void DisplayILI9341::drawVerticalLine(int16_t x, int16_t y, int16_t h) {
	drawVerticalLine(x, y, h, CurrentTextColor);
}

// Draw a vertical line at the given coordinates with the given height and color.
// A vertical line is parallel to the longer side of the rectangular display
// Input: x     horizontal position of the start of the line, columns from the left edge
//        y     vertical position of the start of the line, rows from the top edge
//        h     vertical height of the line
//		color	RGB color of line
void DisplayILI9341::drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color) {
	// safety
	if ((x >= getFrameBuffer()->getBufferWidth()) || (y >= getFrameBuffer()->getBufferHeight()))
		return;
	if ((y + h - 1) >= getFrameBuffer()->getBufferWidth())
		h = getFrameBuffer()->getBufferWidth() - y;
	getFrameBuffer()->drawVerticalLine(x, y, h, color);
}

void DisplayILI9341::drawHorizontalLine(int16_t x, int16_t y, int16_t w) {
	return drawHorizontalLine(x, y, w, CurrentTextColor);
}

// Draw a horizontal line at the given coordinates with the given width and color.
// Input: x     horizontal position of the start of the line, columns from the left edge
//        y     vertical position of the start of the line, rows from the top edge
//        w     horizontal width of the line
//		Color is the RGBColor
void DisplayILI9341::drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor& color) {
	//safey
	if ((x >= getFrameBuffer()->getBufferWidth()) || (y >= getFrameBuffer()->getBufferHeight()))
		return;
	if ((x + w - 1) >= getFrameBuffer()->getBufferWidth())
		w = getFrameBuffer()->getBufferWidth() - x;

	getFrameBuffer()->drawHorizontalLine(x, y, w, color);
}

