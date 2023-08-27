#pragma once

#include "../../error_type.h"
#include "fonts.h"
#include "../../utility/bitarray.h"
#include <cstdint>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "color.h"
#include "hal/gpio_types.h"
#include "display_types.h"
#include "basic_back_buffer.h"

namespace libesp {

class SPIDevice;
class SPIBus;


/*
 * @author cmdc0de
 * @date:  8-19-23
 *
 * Base class for  all displays 
 * Ultimately this new structure will replace ssd1306 and all displays using displaydevice as a base
 *
 * We are going to use the prototype pattern to abstract out the SPI/I2C command differences
 */
template<typename DisplayDT>
class Display {
public:
	Display() :DisplayDeviceType(0), BasicBackBufferType(0), CurrentFont(0)
              , CurrentTextColor(libesp::RGBColor::WHITE), CurrentBGColor(libesp::RGBColor::BLACK) {
   }
	~Display() { }
public:
   ErrorType init(DisplayDT *dt, BasicBackBuffer *bb, const FontDef_t *f, RGBColor currentTextColor
         , RGBColor currentBGColor) {
      ErrorType et;
      //if (!(et = dt->init()).ok()) {
      //   return et;
      //}
      if (!(et = bb->init()).ok()) {
         return et;
      }
      setDisplayType(dt);
      setBasicBackBuffer(bb);
      setFont(f);
      setCurrentTextColor(currentTextColor);
      setCurrentBGColor(currentBGColor);
      return et;
   }
   void setDisplayType(DisplayDT *dt) { DisplayDeviceType = dt; }
   void setBasicBackBuffer(BasicBackBuffer *bb) { BasicBackBufferType = bb; }
	uint16_t getScreenWidth() const {
      return DisplayDeviceType->getWidth();
   }
	uint16_t getScreenHeight() const {
      return DisplayDeviceType->getHeight();
   }
	DISPLAY_ROTATION getRotation() const {
      return DisplayDeviceType->getRotation();
   }
	void setRotation(DISPLAY_ROTATION r) {
      DisplayDeviceType->setRotation(r);
      BasicBackBufferType->setRotation(r);
   }
   void setPixelFormat(LIB_PIXEL_FORMAT pf) {
      BasicBackBufferType->setPixelFormat(pf);
      DisplayDeviceType->setPixelFormat(BasicBackBufferType);
   }
	bool drawPixel(int16_t x0, int16_t y0, const RGBColor &color) {
      return BasicBackBufferType->drawPixel(x0, y0, color);
   }
	void fillRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
      BasicBackBufferType->fillRec(x, y, w, h, color);
   }
	void drawRec(int16_t x, int16_t y, int16_t w, int16_t h, const RGBColor &color) {
      BasicBackBufferType->drawRec(x, y, w, h, color);
   }
	void fillScreen(const RGBColor &color) {
      BasicBackBufferType->fillScreen(color);
   }
	void drawImage(int16_t x, int16_t y, const DCImage &dcImage) {
      BasicBackBufferType->drawImage(x, y, dcImage);
   }
	uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt) {
      return drawString(xPos, yPos, pt, CurrentTextColor);
   }
	uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor) {
      return drawString(xPos, yPos, pt, textColor, CurrentBGColor, 1, false);
   }
	uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor, const RGBColor &bgColor, uint8_t size, bool lineWrap) {
      return drawString(xPos, yPos, pt, textColor, bgColor, size, lineWrap, strlen(pt));
   }
	uint32_t drawString(uint16_t xPos, uint16_t yPos, const char *pt, const RGBColor &textColor, const RGBColor &backGroundColor, uint8_t size, bool lineWrap, uint8_t charsToRender) {
      uint16_t currentX = xPos;
      uint16_t currentY = yPos;
      const char *orig = pt;

      while (charsToRender-- && *pt) {
         if ((currentX > BasicBackBufferType->getBufferWidth() && !lineWrap) 
               || currentY > BasicBackBufferType->getBufferHeight()) {
            return pt - orig;
         } else if (currentX > BasicBackBufferType->getBufferWidth() && lineWrap) {
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
      return pt-orig;
   }

	uint32_t drawStringOnLine(uint8_t line, const char *msg) {
	   return drawString(0, getFont()->FontHeight * line, msg, RGBColor::WHITE, RGBColor::BLACK, 1, true);
   }

   ErrorType setBacklight(uint8_t level) {
      return DisplayDeviceType->backlight(level);
   }

	void drawHorizontalLine(int16_t x, int16_t y, int16_t w) {
      BasicBackBufferType->drawHorizontalLine(x, y, w, CurrentTextColor);
   }
	void drawHorizontalLine(int16_t x, int16_t y, int16_t w, const RGBColor &color) {
      BasicBackBufferType->drawHorizontalLine(x, y, w, color);
   }
	void drawVerticalLine(int16_t x,int16_t y,int16_t h) {
      BasicBackBufferType->drawVerticalLine(x, y, h, CurrentTextColor);
   }
	void drawVerticalLine(int16_t x,int16_t y,int16_t h,const RGBColor &color) {
      BasicBackBufferType->drawVerticalLine(x, y, h, color);
   }
	void swap() {
      DisplayDeviceType->swap(BasicBackBufferType);
   }
	void setFont(const FontDef_t *font) {
      CurrentFont = font;
   }
	const FontDef_t *getFont() {
		return CurrentFont;
	}
	const uint8_t *getFontData() {
      return CurrentFont->data;
   }
   void setCurrentTextColor(const RGBColor &color) {
      CurrentTextColor = color;
   }
   void setCurrentBGColor(const RGBColor &color) {
      CurrentBGColor = color;
   }
protected:
   void drawCharAtPosition(int16_t x, int16_t y, char c, const RGBColor &textColor
         , const RGBColor &bgColor, uint8_t size) {
      uint8_t line; // vertical column of pixels of character in font
      int32_t i, j;
      if ((x >= BasicBackBufferType->getBufferWidth()) || // Clip right
         (y >= BasicBackBufferType->getBufferHeight()) || // Clip bottom
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
private:
   DisplayDT         *DisplayDeviceType;
   BasicBackBuffer   *BasicBackBufferType;
	const FontDef_t   *CurrentFont;
	RGBColor CurrentTextColor;
	RGBColor CurrentBGColor;
};

}
