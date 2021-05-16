#include "ssd1306.h"
#include "../../i2c.hpp"
#include <cstring>

using namespace libesp;

#define ABS(x)   ((x) > 0 ? (x) : -(x))

static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8] = { 0x01 };

class StackController {
public:
	StackController(ESP32_I2CMaster *i2c) : i(i2c) { i->start(SSD1306_I2C_ADDR,true);}
	~StackController() {i->stop(10);}
private:
	ESP32_I2CMaster *i;
};

void SSD1306::writeCommand(uint8_t cmd) {
	write(0x00,cmd);
}

void SSD1306::writeData(uint8_t data) {
	write(0x40,data);
}

void SSD1306::writeMulti(uint8_t reg, uint8_t* data, uint16_t count) {
	StackController sc(I2CDisplay);
	I2CDisplay->write(&reg,1,true);
	I2CDisplay->write(data,count,false);
}

void SSD1306::write(uint8_t reg, uint8_t data) {
	StackController sc(I2CDisplay);
	uint8_t dt[2];
	dt[0] = reg;
	dt[1] = data;
	I2CDisplay->write(&dt[0],2,true);
}

/* SSD1306 data buffer */

SSD1306::SSD1306() : CurrentX(0), CurrentY(0), Inverted(0), I2CDisplay(nullptr) {

}

bool SSD1306::init(ESP32_I2CMaster *i2c, bool bEnablePullUps) {
	I2CDisplay = i2c;

	if(!i2c->init(bEnablePullUps) || !i2c->start(SSD1306_I2C_ADDR,true) || !i2c->stop(1000)) {
		I2CDisplay = nullptr;
		return false;
	}

	/* Init LCD */
	writeCommand(0xAE); //display off
	writeCommand(0x20); //Set Memory Addressing Mode   
	writeCommand(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	writeCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
	writeCommand(0xC8); //Set COM Output Scan Direction
	writeCommand(0x00); //---set low column address
	writeCommand(0x10); //---set high column address
	writeCommand(0x40); //--set start line address
	writeCommand(0x81); //--set contrast control register
	writeCommand(0xFF);
	writeCommand(0xA1); //--set segment re-map 0 to 127
	writeCommand(0xA6); //--set normal display
	writeCommand(0xA8); //--set multiplex ratio(1 to 64)
	writeCommand(0x3F); //
	writeCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	writeCommand(0xD3); //-set display offset
	writeCommand(0x00); //-not offset
	writeCommand(0xD5); //--set display clock divide ratio/oscillator frequency
	writeCommand(0xF0); //--set divide ratio
	writeCommand(0xD9); //--set pre-charge period
	writeCommand(0x22); //
	writeCommand(0xDA); //--set com pins hardware configuration
	writeCommand(0x12);
	writeCommand(0xDB); //--set vcomh
	writeCommand(0x20); //0x20,0.77xVcc
	writeCommand(0x8D); //--set DC-DC enable
	writeCommand(0x14); //
	writeCommand(0xAF); //--turn on SSD1306 panel

	/* Update screen */
	updateScreen();

	/* Set default values */
	CurrentX = 0;
	CurrentY = 0;
	/* Return OK */
	return true;
}

void SSD1306::updateScreen(void) {
	uint8_t m;

	for (m = 0; m < 8; m++) {
		writeCommand(0xB0 + m);
		writeCommand(0x00);
		writeCommand(0x10);

		/* Write multi data */
		writeMulti(0x40, &SSD1306_Buffer[SSD1306_WIDTH * m], SSD1306_WIDTH + 1);
	}
}

void SSD1306::toggleInvert(void) {
	/* Toggle invert */
	Inverted = !Inverted;
	/* Do memory toggle */
	for (uint16_t i = 0; i < sizeof(SSD1306_Buffer); i++) {
		SSD1306_Buffer[i] = ~SSD1306_Buffer[i];
	}
}

void SSD1306::fill(uint8_t color) {
	/* Set memory */
	memset(SSD1306_Buffer, (color == COLOR_BLACK) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void SSD1306::drawPixel(uint16_t x, uint16_t y, uint8_t color) {
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
		/* Error */
		return;
	}

	/* Check if pixels are inverted */
	if (Inverted) {
		color = (uint8_t) !color;
	}

	/* Set color */
	if (color == COLOR_WHITE) {
		SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
	} else {
		SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
	}
}

void SSD1306::gotoXY(uint16_t x, uint16_t y) {
	/* Set write pointers */
	CurrentX = x;
	CurrentY = y;
}

char SSD1306::putc(char ch, WFontDef_t* Font, uint8_t color) {
	uint32_t b = 0;
	/* Go through font */
	for (uint32_t i = 0; i < Font->FontHeight; i++) {
		b = Font->data[(ch - 32) * Font->FontHeight + i];
		for (uint32_t j = 0; j < Font->FontWidth; j++) {
			if ((b << j) & 0x8000) {
				drawPixel(CurrentX + j, (CurrentY + i), color);
			} else {
				drawPixel(CurrentX + j, (CurrentY + i), !color);
			}
		}
	}

	/* Increase pointer */
	CurrentX += Font->FontWidth;

	/* Return character written */
	return ch;
}

char SSD1306::puts(const char* str, WFontDef_t* Font, uint8_t color) {
	/* Write characters */
	while (*str) {
		/* Write character by character */
		if (putc(*str, Font, color) != *str) {
			/* Return error */
			return *str;
		}
		str++;
	}
	/* Everything OK, zero should be returned */
	return *str;
}

void SSD1306::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t c) {
	int16_t dx, dy, sx, sy, err, e2, i, tmp;

	/* Check for overflow */
	if (x0 >= SSD1306_WIDTH) {
		x0 = SSD1306_WIDTH - 1;
	}
	if (x1 >= SSD1306_WIDTH) {
		x1 = SSD1306_WIDTH - 1;
	}
	if (y0 >= SSD1306_HEIGHT) {
		y0 = SSD1306_HEIGHT - 1;
	}
	if (y1 >= SSD1306_HEIGHT) {
		y1 = SSD1306_HEIGHT - 1;
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
			drawPixel(x0, i, c);
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
			drawPixel(i, y0, c);
		}
		return;
	}

	while (1) {
		drawPixel(x0, y0, c);
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

void SSD1306::drawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t c) {
	/* Check input parameters */
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH) {
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT) {
		h = SSD1306_HEIGHT - y;
	}

	/* Draw 4 lines */
	drawLine(x, y, x + w, y, c); /* Top line */
	drawLine(x, y + h, x + w, y + h, c); /* Bottom line */
	drawLine(x, y, x, y + h, c); /* Left line */
	drawLine(x + w, y, x + w, y + h, c); /* Right line */
}

void SSD1306::drawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t c) {
	uint8_t i;

	/* Check input parameters */
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH) {
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT) {
		h = SSD1306_HEIGHT - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		drawLine(x, y + i, x + w, y + i, c);
	}
}

void SSD1306::drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
	/* Draw lines */
	drawLine(x1, y1, x2, y2, color);
	drawLine(x2, y2, x3, y3, color);
	drawLine(x3, y3, x1, y1, color);
}

void SSD1306::drawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0, yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd =
			0, numpixels = 0, curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	} else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	} else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) {
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		drawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

void SSD1306::drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	drawPixel(x0, y0 + r, c);
	drawPixel(x0, y0 - r, c);
	drawPixel(x0 + r, y0, c);
	drawPixel(x0 - r, y0, c);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		drawPixel(x0 + x, y0 + y, c);
		drawPixel(x0 - x, y0 + y, c);
		drawPixel(x0 + x, y0 - y, c);
		drawPixel(x0 - x, y0 - y, c);

		drawPixel(x0 + y, y0 + x, c);
		drawPixel(x0 - y, y0 + x, c);
		drawPixel(x0 + y, y0 - x, c);
		drawPixel(x0 - y, y0 - x, c);
	}
}

void SSD1306::drawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	drawPixel(x0, y0 + r, c);
	drawPixel(x0, y0 - r, c);
	drawPixel(x0 + r, y0, c);
	drawPixel(x0 - r, y0, c);
	drawLine(x0 - r, y0, x0 + r, y0, c);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		drawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
		drawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);
		drawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
		drawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
	}
}

void SSD1306::image(uint8_t *img, uint8_t x, uint8_t y) {
	uint32_t b = 0;
	for (uint32_t i = 0; i < img[1]; i++) {
		for (uint32_t j = 0; j < img[0]; j++) {
			drawPixel(x + j, (y + i),(uint8_t)(img[b/8+2] >> (b % 8)) & 1);
			b++;
		}
	}
}

/*
void SSD1306_ON(void) {
	writeCommand(0x8D);
	writeCommand(0x14);
	writeCommand(0xAF);
}
void SSD1306_OFF(void) {
	writeCommand(0x8D);
	writeCommand(0x10);
	writeCommand(0xAE);
}
*/
