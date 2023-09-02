#pragma once

#include "../../freertos.h"
#include "../../error_type.h"
#include "fonts.h"
#include "../../utility/bitarray.h"
#include <cstdint>
#include <driver/gpio.h>
#include "color.h"
#include "hal/gpio_types.h"
#include "display_types.h"
#include <driver/ledc.h>
#include "../../spibus.h"

namespace libesp {

class SPIDevice;
class SPIBus;
class BasicBackBuffer;

// GC901 is a 240x240 display
// @author cmdc0de
// @date 8-19-2023
// tested on the round display
class GC9A01 {
   public:
      static ErrorType spiInit(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk, int channel
            , spi_host_device_t spiNum);
   public:
      GC9A01(DISPLAY_ROTATION r);
      ~GC9A01();
      static const char *LOGTAG;
   public: //commands
		static const uint8_t Cmd_SLPIN                     = 0x10;
		static const uint8_t Cmd_SLPOUT                    = 0x11;
		static const uint8_t Cmd_INVOFF                    = 0x20;
		static const uint8_t Cmd_INVON                     = 0x21;
		static const uint8_t Cmd_DISPOFF                   = 0x28;
		static const uint8_t Cmd_DISPON                    = 0x29;
		static const uint8_t Cmd_CASET                     = 0x2A;
		static const uint8_t Cmd_RASET                     = 0x2B;
		static const uint8_t Cmd_RAMWR                     = 0x2C;
		static const uint8_t Cmd_TEON      	               = 0x35;   // Tearing effect line ON
		static const uint8_t Cmd_MADCTL                    = 0x36;   // Memory data access control
		static const uint8_t Cmd_COLMOD                    = 0x3A;  // Pixel format set
		static const uint8_t Cmd_DisplayFunctionControl    = 0xB6;
		static const uint8_t Cmd_PWCTR1                    = 0xC1;   // Power control 1
		static const uint8_t Cmd_PWCTR2                    = 0xC3;   // Power control 2
		static const uint8_t Cmd_PWCTR3                    = 0xC4;   // Power control 3
		static const uint8_t Cmd_PWCTR4                    = 0xC9;   // Power control 4
		static const uint8_t Cmd_PWCTR7                    = 0xA7;   // Power control 7
		static const uint8_t Cmd_FRAMERATE                 = 0xE8;
		static const uint8_t Cmd_InnerReg1Enable           = 0xFE;
		static const uint8_t Cmd_InnerReg2Enable           = 0xEF;
		static const uint8_t Cmd_GAMMA1                    = 0xF0;   // Set gamma 1
		static const uint8_t Cmd_GAMMA2                    = 0xF1;    // Set gamma 2
		static const uint8_t Cmd_GAMMA3                    = 0xF2;    // Set gamma 3
		static const uint8_t Cmd_GAMMA4                    = 0xF3;    // Set gamma 4
		static const uint8_t ColorMode_RGB_16bit           = 0x50;
		static const uint8_t ColorMode_RGB_18bit           = 0x60;
		static const uint8_t ColorMode_MCU_12bit           = 0x03;
		static const uint8_t ColorMode_MCU_16bit           = 0x05;
		static const uint8_t ColorMode_MCU_18bit           = 0x06;
		static const uint8_t MADCTL_MY                     = 0x80;
		static const uint8_t MADCTL_MX                     = 0x40;
		static const uint8_t MADCTL_MV                     = 0x20;
		static const uint8_t MADCTL_ML                     = 0x10;
		static const uint8_t MADCTL_BGR                    = 0x08;
      static const uint8_t MADCTL_MH                     = 0x04;
   public:
      ErrorType init(SPIBus *bus, gpio_num_t cs, gpio_num_t dataCmdPin, gpio_num_t resetPin
      , gpio_num_t backlightPin, BasicBackBuffer *bb, SemaphoreHandle_t handle);
      ///////
      //calling initBackLightPWM will allow you to set the backlight to be PWM'ed
      ////////
      ErrorType initBackLightPWM(ledc_timer_t led_timer, ledc_channel_t channel);
      bool setRotation(DISPLAY_ROTATION rotation);
      DISPLAY_ROTATION getRotation() const { return Rotation; }
      ErrorType backlight(uint16_t level);
      void reset();
      bool setPixelFormat(const BasicBackBuffer *backBuff);
      ErrorType setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
      uint16_t getWidth() const { return DisplayWidth; }
      uint16_t getHeight() const { return DisplayHeight; }
      ErrorType swap(BasicBackBuffer *backBuffer);
      void sleepMode(bool sleep);
      void inversionMode(bool invert);
      void powerDisplay(bool On);
   private:
      union DisplayFlags {
         struct {
            uint8_t BackLightPWM:1;
            uint8_t VerticalMirror:1;
            uint8_t HorizontalMirror:1;
            uint8_t IsBGR:1;
         };
         uint16_t Flags;
      };
   protected:
	   bool writeCmd(uint8_t c);
	   bool writeNData(const uint8_t *data, int nbytes);
	   bool write16Data(const uint16_t &data);
	   bool writeN(char dc, const uint8_t *data, int nbytes);
      bool memAccessModeSet(DISPLAY_ROTATION Rotation, bool VertMirror, bool HorizMirror, bool IsBGR);
      void rowSet(uint16_t RowStart, uint16_t RowEnd);
      void columnSet(uint16_t ColumnStart, uint16_t ColumnEnd);
      bool supportBackLightPWM() const { return Flags.BackLightPWM; }
   private:
      SPIDevice *SPI;
      gpio_num_t PinDC;
      gpio_num_t PinRST;
      gpio_num_t PinBL;
      uint16_t DisplayWidth;
      uint16_t DisplayHeight;
      DISPLAY_ROTATION Rotation;
      SemaphoreHandle_t SpiSemaphoreHandle;
      DisplayFlags Flags;
      ledc_channel_config_t  Ledc_cConfig;
      ledc_timer_config_t    Ledc_tConfig;
};

}

