/*
 * ssd1306_display.h
 *
 * Templated SSD1306 OLED display driver inheriting from IDisplay.
 * Template parameter selects I2C or SPI transport.
 *
 * Usage:
 *   libesp::SSD1306_I2C oled;
 *   oled.init(&i2cMaster, &Font_6x10, 0x3C);
 *
 *   libesp::SSD1306_SPI oled;
 *   oled.init(spiBus, csPin, dcPin, &Font_6x10, resetPin);
 */

#pragma once

#include "display.h"
#include "fonts.h"
#include "color.h"
#include "../../error_type.h"
#include "../../i2c.hpp"
#include "../../spidevice.h"
#include "../../spibus.h"
#include <cstring>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_attr.h>
#include <esp_log.h>

namespace libesp {

///////////////////////////////////////////////////////////////////////////////
// Init sequence (defined in ssd1306_display.cpp)
///////////////////////////////////////////////////////////////////////////////
struct SSD1306InitCmd {
	uint8_t cmd;
};

extern const SSD1306InitCmd SSD1306_INIT_SEQUENCE[];
extern const uint16_t SSD1306_INIT_SEQUENCE_LEN;

///////////////////////////////////////////////////////////////////////////////
// I2C Transport Policy
///////////////////////////////////////////////////////////////////////////////
class SSD1306_I2CTransport {
public:
	SSD1306_I2CTransport() : I2C(nullptr), Address(0x3C) {}

	ErrorType init(ESP32_I2CMaster *i2c, uint8_t addr = 0x3C) {
		I2C = i2c;
		Address = addr;
		return ErrorType();
	}

	void writeCommand(uint8_t cmd) {
		write(0x00, cmd);
	}

	void writeData(uint8_t data) {
		write(0x40, data);
	}

	void writeMultiData(const uint8_t *data, uint16_t len) {
		I2C->start(Address, true);
		uint8_t reg = 0x40;
		I2C->write(&reg, 1, true);
		I2C->write(const_cast<uint8_t*>(data), len, false);
		I2C->stop(10);
	}

private:
	void write(uint8_t reg, uint8_t data) {
		I2C->start(Address, true);
		uint8_t dt[2] = {reg, data};
		I2C->write(&dt[0], 2, true);
		I2C->stop(10);
	}

	ESP32_I2CMaster *I2C;
	uint8_t Address;
};

///////////////////////////////////////////////////////////////////////////////
// SPI Transport Policy
///////////////////////////////////////////////////////////////////////////////
class SSD1306_SPITransport {
public:
	SSD1306_SPITransport()
		: SPI(nullptr), PinDC(GPIO_NUM_NC), PinRST(GPIO_NUM_NC) {}

	ErrorType init(SPIBus *bus, gpio_num_t cs, gpio_num_t dc,
			gpio_num_t reset = GPIO_NUM_NC,
			SemaphoreHandle_t spiSemaphore = nullptr) {
		ErrorType et;
		PinDC = dc;
		PinRST = reset;
		DCPinGlobal = dc;

		// Configure DC pin as output
		gpio_config_t conf = {};
		conf.intr_type = GPIO_INTR_DISABLE;
		conf.mode = GPIO_MODE_OUTPUT;
		conf.pin_bit_mask = (1ULL << PinDC);
		conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
		conf.pull_up_en = GPIO_PULLUP_DISABLE;
		et = gpio_config(&conf);
		if (!et.ok()) return et;

		// Configure and toggle reset pin if present
		if (PinRST != GPIO_NUM_NC) {
			conf.pin_bit_mask = (1ULL << PinRST);
			et = gpio_config(&conf);
			if (!et.ok()) return et;
			gpio_set_level(PinRST, 1);
			vTaskDelay(10 / portTICK_PERIOD_MS);
			gpio_set_level(PinRST, 0);
			vTaskDelay(10 / portTICK_PERIOD_MS);
			gpio_set_level(PinRST, 1);
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}

		// Create SPI master device
		spi_device_interface_config_t devcfg = {};
		devcfg.clock_speed_hz = 8 * 1000 * 1000;
		devcfg.mode = 0;
		devcfg.spics_io_num = cs;
		devcfg.queue_size = 5;
		devcfg.pre_cb = ssd1306SpiPreTransferCb;

		SPI = bus->createMasterDevice(devcfg, spiSemaphore);
		if (!SPI) {
			return ErrorType(ESP_ERR_NO_MEM);
		}
		return et;
	}

	void writeCommand(uint8_t cmd) {
		void *ud = reinterpret_cast<void*>(0);
		SPI->send(&cmd, 1, ud);
	}

	void writeData(uint8_t data) {
		void *ud = reinterpret_cast<void*>(1);
		SPI->send(&data, 1, ud);
	}

	void writeMultiData(const uint8_t *data, uint16_t len) {
		void *ud = reinterpret_cast<void*>(1);
		SPI->send(data, len, ud);
	}

	static gpio_num_t DCPinGlobal;

private:
	static IRAM_ATTR void ssd1306SpiPreTransferCb(spi_transaction_t *t) {
		int dc = reinterpret_cast<int>(t->user);
		gpio_set_level(DCPinGlobal, dc);
		t->user = nullptr;
	}

	SPIDevice *SPI;
	gpio_num_t PinDC;
	gpio_num_t PinRST;
};

///////////////////////////////////////////////////////////////////////////////
// SSD1306Display<Transport> - DisplayDevice implementation
///////////////////////////////////////////////////////////////////////////////
template<typename Transport>
class SSD1306Display {
public:
	typedef SSD1306Display<Transport> SELF_TYPE;
public:
	static constexpr uint16_t WIDTH = 128;
	static constexpr uint16_t HEIGHT = 64;
	static constexpr uint32_t BUFFER_SIZE = WIDTH * HEIGHT / 8; // 1024
	static const char *LOGTAG;

public:
	SSD1306Display();
	~SSD1306Display();

	// I2C init
	ErrorType init(ESP32_I2CMaster *i2c, const FontDef_t *defaultFont, uint8_t addr = 0x3C);
	// SPI init
	ErrorType init(SPIBus *bus, gpio_num_t cs, gpio_num_t dc, const FontDef_t *defaultFont,
			gpio_num_t reset = GPIO_NUM_NC, SemaphoreHandle_t sema = nullptr);

	//adapters
	IDisplayMessageDisplay *getDMSAdapter();
	// DisplayDevice pure virtual implementations
	bool drawPixel(int16_t x0, int16_t y0, const RGBColor &color);
	void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	void drawRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color);
	void fillScreen(const RGBColor &color);
	void drawImage(int16_t x, int16_t y, const DCImage &dcImage);
   void drawLine(int x0, int y0, int x1, int y1, RGBColor& color);

	uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt);
	uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor);
	uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt,
			const RGBColor &textColor, const RGBColor &bgColor, uint8_t size, bool lineWrap);
	uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt,
			const RGBColor &textColor, const RGBColor &backGroundColor, uint8_t size, bool lineWrap, uint8_t charsToRender);
	uint32_t drawStringOnLine(uint8_t line, const char *msg);

	void drawHorizontalLine(int16_t x, int16_t y, int16_t w);
	void drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor &color);
	void drawVerticalLine(int16_t x, int16_t y, int16_t h);
	void drawVerticalLine(int16_t x, int16_t y, int16_t h, const RGBColor &color);
	void swap() ;

	// SSD1306-specific
	void setContrast(uint8_t value);
	void setInverted(bool inverted);
	void displayOn(bool on);

	Transport &getTransport() { return Bus; }
	void setFont(const FontDef_t *f) {
		DefaultFont = f;
	}
	const FontDef_t *getFont() const {
		return DefaultFont;
	}
	const uint8_t *getFontData() const {
		return DefaultFont->data;
	}
	const RGBColor &getDefaultTextColor() const {
		return CurrentTextColor;
	}
	const RGBColor &getDefaultBackgroundColor() const {
		return CurrentBGColor;
	}
	ErrorType setBacklight(uint8_t l) {
		//unless we PWN the power line can't really dim this screen
		return ErrorType();
	}
	uint16_t getFrameBufferWidth() const {
		return SELF_TYPE::WIDTH;
	}
	uint16_t getFrameBufferHeight() const {
		return SELF_TYPE::HEIGHT;
	}
	uint16_t getCanvasWidth() const {
		return SELF_TYPE::WIDTH;
	}
	uint16_t getCanvasHeight() const {
		return SELF_TYPE::HEIGHT;
	}
private:
	ErrorType initDisplay(const FontDef_t *defaultFont);
	void sendInitSequence();
	void drawCharAtPosition(int16_t x, int16_t y, char c,
			const RGBColor &textColor, const RGBColor &bgColor, uint8_t size);
	bool isPixelOn(const RGBColor &color) const;
	void setBufferPixel(int16_t x, int16_t y, bool on);

private:
	IDisplayMessageDisplay *MyDMSAdapter;	
	Transport Bus;
	uint8_t Buffer[BUFFER_SIZE];
	RGBColor CurrentTextColor;
	RGBColor CurrentBGColor;
	bool Inverted;
	const FontDef_t *DefaultFont;
};

///////////////////////////////////////////////////////////////////////////////
// Template implementations
///////////////////////////////////////////////////////////////////////////////

template<typename Transport>
const char *SSD1306Display<Transport>::LOGTAG = "SSD1306Display";

template<typename Transport>
SSD1306Display<Transport>::SSD1306Display()
	: MyDMSAdapter(nullptr)
	, Bus()
	, CurrentTextColor(RGBColor::WHITE)
	, CurrentBGColor(RGBColor::BLACK)
	, Inverted(false) {
	memset(Buffer, 0, BUFFER_SIZE);
}

template<typename Transport>
SSD1306Display<Transport>::~SSD1306Display() {
	delete MyDMSAdapter;
}	

// I2C init
template<typename Transport>
ErrorType SSD1306Display<Transport>::init(ESP32_I2CMaster *i2c, const FontDef_t *defaultFont, uint8_t addr) {
	ErrorType et = Bus.init(i2c, addr);
	if (!et.ok()) {
		ESP_LOGE(LOGTAG, "I2C init error: %d %s", et.getErrT(), et.toString());
		return et;
	}
	return initDisplay(defaultFont);
}

// SPI init
template<typename Transport>
ErrorType SSD1306Display<Transport>::init(SPIBus *bus, gpio_num_t cs,
		gpio_num_t dc, const FontDef_t *defaultFont,
		gpio_num_t reset, SemaphoreHandle_t sema) {
	ErrorType et = Bus.init(bus, cs, dc, reset, sema);
	if (!et.ok()) return et;
	return initDisplay(defaultFont);
}

template<typename Transport>
class DMSAdapter: public IDisplayMessageDisplay {
public:
	DMSAdapter(SSD1306Display<Transport> *display) : AdapterDisplay(display) {
	}
	virtual void clearScreen() override {
		AdapterDisplay->fillScreen(RGBColor::BLACK);
	}
	virtual void drawString(uint16_t xPos, uint16_t yPos, const char *msg, const RGBColor &t, const RGBColor &b
			, uint16_t sizeMultiplier, bool wrapMessage) override {
		AdapterDisplay->drawString(xPos, yPos, msg, t, b, sizeMultiplier, wrapMessage);
	}
	virtual ~DMSAdapter() { }
private:
	SSD1306Display<Transport> *AdapterDisplay;
};

template<typename Transport>
IDisplayMessageDisplay *SSD1306Display<Transport>::getDMSAdapter() {
	if (nullptr==MyDMSAdapter) {
		MyDMSAdapter = new DMSAdapter(this);
	}
	return MyDMSAdapter;
}

template<typename Transport>
ErrorType SSD1306Display<Transport>::initDisplay(const FontDef_t* defaultFont) {
	setFont(defaultFont);
	sendInitSequence();
	fillScreen(RGBColor::BLACK);
	swap();
	return ErrorType();
}

template<typename Transport>
void SSD1306Display<Transport>::sendInitSequence() {
	for (uint16_t i = 0; i < SSD1306_INIT_SEQUENCE_LEN; ++i) {
		Bus.writeCommand(SSD1306_INIT_SEQUENCE[i].cmd);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Pixel / buffer operations
///////////////////////////////////////////////////////////////////////////////

template<typename Transport>
bool SSD1306Display<Transport>::isPixelOn(const RGBColor &color) const {
	return color != RGBColor::BLACK;
}

template<typename Transport>
void SSD1306Display<Transport>::setBufferPixel(int16_t x, int16_t y, bool on) {
	if (on) {
		Buffer[x + (y / 8) * WIDTH] |= (1 << (y % 8));
	} else {
		Buffer[x + (y / 8) * WIDTH] &= ~(1 << (y % 8));
	}
}

template<typename Transport>
bool SSD1306Display<Transport>::drawPixel(int16_t x0, int16_t y0,
		const RGBColor &color) {
	if (x0 < 0 || x0 >= WIDTH || y0 < 0 || y0 >= HEIGHT)
		return false;

	bool on = isPixelOn(color);
	if (Inverted) on = !on;
	setBufferPixel(x0, y0, on);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Rectangles
///////////////////////////////////////////////////////////////////////////////
template<typename Transport>
void SSD1306Display<Transport>::drawLine(int x0, int y0, int x1, int y1, RGBColor& color) {
	int16_t dx, dy, sx, sy, err, e2, i, tmp;

	/* Check for overflow */
	if (x0 >= WIDTH) {
		x0 = WIDTH - 1;
	}
	if (x1 >= WIDTH) {
		x1 = WIDTH - 1;
	}
	if (y0 >= HEIGHT) {
		y0 = HEIGHT - 1;
	}
	if (y1 >= HEIGHT) {
		y1 = HEIGHT - 1;
	}

	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
	sx = (x0 < x1) ? 1 : -1;
	sy = (y0 < y1) ? 1 : -1;
	err = ((dx > dy) ? dx : -dy) / 2;

	if (dx == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}

		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}

		/* Vertical line */
		for (i = y0; i <= y1; i++) {
			drawPixel(x0, i, color);
		}

		/* Return from function */
		return;
	}

	if (dy == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		/* Horizontal line */
		for (i = x0; i <= x1; i++) {
			drawPixel(i, y0, color);
		}
		return;
	}

	while (1) {
		drawPixel(x0, y0, color);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}


template<typename Transport>
void SSD1306Display<Transport>::fillRec(int16_t x, int16_t y,
		int16_t w, int16_t h, const RGBColor &color) {
	if (x >= WIDTH || y >= HEIGHT) return;
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > WIDTH)  w = WIDTH - x;
	if (y + h > HEIGHT) h = HEIGHT - y;
	if (w <= 0 || h <= 0) return;

	bool on = isPixelOn(color);
	if (Inverted) on = !on;

	for (int16_t j = y; j < y + h; ++j) {
		for (int16_t i = x; i < x + w; ++i) {
			setBufferPixel(i, j, on);
		}
	}
}

template<typename Transport>
void SSD1306Display<Transport>::drawRec(int16_t x, int16_t y,
		int16_t w, int16_t h, const RGBColor &color) {
	drawHorizontalLine(x, y, w, color);
	drawHorizontalLine(x, y + h - 1, w, color);
	drawVerticalLine(x, y, h, color);
	drawVerticalLine(x + w - 1, y, h, color);
}

template<typename Transport>
void SSD1306Display<Transport>::fillScreen(const RGBColor &color) {
	bool on = isPixelOn(color);
	if (Inverted) on = !on;
	memset(Buffer, on ? 0xFF : 0x00, BUFFER_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
// Lines
///////////////////////////////////////////////////////////////////////////////

template<typename Transport>
void SSD1306Display<Transport>::drawHorizontalLine(int16_t x, int16_t y,
		int16_t w) {
	drawHorizontalLine(x, y, w, CurrentTextColor);
}

template<typename Transport>
void SSD1306Display<Transport>::drawHorizontalLine(int16_t x, int16_t y,
		int16_t w, const RGBColor &color) {
	if (y < 0 || y >= HEIGHT || x >= WIDTH) return;
	if (x < 0) { w += x; x = 0; }
	if (x + w > WIDTH) w = WIDTH - x;
	if (w <= 0) return;

	bool on = isPixelOn(color);
	if (Inverted) on = !on;
	for (int16_t i = x; i < x + w; ++i) {
		setBufferPixel(i, y, on);
	}
}

template<typename Transport>
void SSD1306Display<Transport>::drawVerticalLine(int16_t x, int16_t y,
		int16_t h) {
	drawVerticalLine(x, y, h, CurrentTextColor);
}

template<typename Transport>
void SSD1306Display<Transport>::drawVerticalLine(int16_t x, int16_t y,
		int16_t h, const RGBColor &color) {
	if (x < 0 || x >= WIDTH || y >= HEIGHT) return;
	if (y < 0) { h += y; y = 0; }
	if (y + h > HEIGHT) h = HEIGHT - y;
	if (h <= 0) return;

	bool on = isPixelOn(color);
	if (Inverted) on = !on;
	for (int16_t i = y; i < y + h; ++i) {
		setBufferPixel(x, i, on);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Image
///////////////////////////////////////////////////////////////////////////////

template<typename Transport>
void SSD1306Display<Transport>::drawImage(int16_t x, int16_t y,
		const DCImage &dcImage) {
	if (dcImage.bytes_per_pixel <= 1) {
		// 1bpp packed format
		uint32_t b = 0;
		for (uint32_t row = 0; row < dcImage.height; ++row) {
			for (uint32_t col = 0; col < dcImage.width; ++col) {
				bool on = (dcImage.pixel_data[b / 8] >> (b % 8)) & 1;
				drawPixel(x + col, y + row,
						on ? RGBColor::WHITE : RGBColor::BLACK);
				++b;
			}
		}
	} else {
		// RGB image: convert to mono
		for (uint32_t row = 0; row < dcImage.height; ++row) {
			for (uint32_t col = 0; col < dcImage.width; ++col) {
				uint32_t idx = (row * dcImage.width + col) * dcImage.bytes_per_pixel;
				bool on = false;
				for (uint32_t c = 0; c < dcImage.bytes_per_pixel; ++c) {
					if (dcImage.pixel_data[idx + c] != 0) {
						on = true;
						break;
					}
				}
				drawPixel(x + col, y + row,
						on ? RGBColor::WHITE : RGBColor::BLACK);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Text rendering (follows TFTDisplay pattern from display_device.cpp)
///////////////////////////////////////////////////////////////////////////////

template<typename Transport>
void SSD1306Display<Transport>::drawCharAtPosition(int16_t x, int16_t y,
		char c, const RGBColor &textColor, const RGBColor &bgColor,
		uint8_t size) {
	uint8_t line;
	int32_t i, j;
	if ((x >= WIDTH) || (y >= HEIGHT) ||
			((x + 5 * size - 1) < 0) || ((y + 8 * size - 1) < 0))
		return;

	for (i = 0; i < getFont()->FontWidth; i++) {
		if (i == getFont()->FontWidth - 1)
			line = 0x0;
		else
			line = getFontData()[(c * getFont()->CharBytes) + i];
		for (j = 0; j < 8; j++) {
			if (line & 0x1) {
				if (size == 1)
					drawPixel(x + i, y + j, textColor);
				else
					fillRec(x + (i * size), y + (j * size), size, size, textColor);
			} else if (bgColor != textColor) {
				if (size == 1)
					drawPixel(x + i, y + j, bgColor);
				else
					fillRec(x + i * size, y + j * size, size, size, bgColor);
			}
			line >>= 1;
		}
	}
}

template<typename Transport>
uint32_t SSD1306Display<Transport>::drawStringOnLine(uint8_t line, const char *msg) {
	return drawString(0, getFont()->FontHeight * line, msg, RGBColor::WHITE, RGBColor::BLACK, 1, true);
}

template<typename Transport>
uint32_t SSD1306Display<Transport>::drawString(uint16_t x, uint16_t y, const char *pt) {
	return drawString(x, y, pt, CurrentTextColor);
}

template<typename Transport>
uint32_t SSD1306Display<Transport>::drawString(uint16_t x, uint16_t y, const char *pt, const RGBColor &textColor) {
	return drawString(x, y, pt, textColor, CurrentBGColor, 1, false);
}

template<typename Transport>
uint32_t SSD1306Display<Transport>::drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor,
		const RGBColor &backGroundColor, uint8_t size, bool lineWrap) {
	uint16_t currentX = xPos;
	uint16_t currentY = yPos;
	const char *orig = pt;

	while (*pt) {
		if ((currentX > WIDTH && !lineWrap) || currentY > HEIGHT) {
			return pt - orig;
		} else if (currentX > WIDTH && lineWrap) {
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
	return (pt - orig);
}

template<typename Transport>
uint32_t SSD1306Display<Transport>::drawString(uint16_t xPos, uint16_t yPos,
		const char *pt, const RGBColor &textColor,
		const RGBColor &backGroundColor, uint8_t size, bool lineWrap,
		uint8_t charsToRender) {
	uint16_t currentX = xPos;
	uint16_t currentY = yPos;
	const char *orig = pt;

	while (charsToRender-- && *pt) {
		if ((currentX > WIDTH && !lineWrap) || currentY > HEIGHT) {
			return pt - orig;
		} else if (currentX > WIDTH && lineWrap) {
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
	return (pt - orig);
}

///////////////////////////////////////////////////////////////////////////////
// swap - flush buffer to display
///////////////////////////////////////////////////////////////////////////////

template<typename Transport>
void SSD1306Display<Transport>::swap() {
	for (uint8_t page = 0; page < 8; page++) {
		Bus.writeCommand(0xB0 + page);
		Bus.writeCommand(0x00);
		Bus.writeCommand(0x10);
		Bus.writeMultiData(&Buffer[WIDTH * page], WIDTH);
	}
}

///////////////////////////////////////////////////////////////////////////////
// SSD1306-specific commands
///////////////////////////////////////////////////////////////////////////////

template<typename Transport>
void SSD1306Display<Transport>::setContrast(uint8_t value) {
	Bus.writeCommand(0x81);
	Bus.writeCommand(value);
}

template<typename Transport>
void SSD1306Display<Transport>::setInverted(bool inverted) {
	Inverted = inverted;
	Bus.writeCommand(inverted ? 0xA7 : 0xA6);
}

template<typename Transport>
void SSD1306Display<Transport>::displayOn(bool on) {
	Bus.writeCommand(on ? 0xAF : 0xAE);
}

///////////////////////////////////////////////////////////////////////////////
// Convenience type aliases
///////////////////////////////////////////////////////////////////////////////

using SSD1306_I2C = SSD1306Display<SSD1306_I2CTransport>;
using SSD1306_SPI = SSD1306Display<SSD1306_SPITransport>;
using SSD1306_I2C_DISPLAY = DisplayInterface<SSD1306_I2C>;

} // namespace libesp
