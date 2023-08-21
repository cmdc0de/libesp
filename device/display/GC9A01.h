#pragma once

#include "../../error_type.h"
#include "fonts.h"
#include "../../utility/bitarray.h"
#include <cstdint>
#include <driver/gpio.h>
#include "color.h"
#include "hal/gpio_types.h"
#include "display_types.h"

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
      static ErrorType spiInit(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk, uint8_t *spiBuffer
            , uint16_t spiBufferSize);
   public:
      GC901(DISPLAY_ROTATION r);
      ~GC901();
   public:
      static const uint8_t DISPLAY_FORMAT_12_BIT = 0b011; 
      static const uint8_t DISPLAY_FORMAT_16_BIT = 0b101;
      static const uint8_t DISPLAY_FORMAT_18_BIT = 0b110;
   public:
      ErrorType init();
	   ErrorType createInitDevice(SPIBus *bus, gpio_num_t cs, gpio_num_t data_cmd);
      ErrorType createInitDevice(SPIBus *bus, gpio_num_t cs, gpio_num_t data_cmd, SemaphoreHandle_t handle);
      ErrorType setRotation(DISPLAY_ROTATION rotation);
      ErrorType sendData(const uint8_t *data, uint16_t len);
      ErrorType backlight(uint8_t level);
      ErrorType reset();
      void setPixelFormat(BasicBackBuffer *backBuff);
      ErrorType setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
      uint16_t getWidth() const { return DisplayWidth; }
      uint16_t getHeight() const { return DisplayHeight; }
      ErrorType swap(BasicBackBuffer *backBuffer);
   protected:
	   bool writeCmd(uint8_t c);
	   bool writeNData(const uint8_t *data, int nbytes);
	   bool write16Data(const uint16_t &data);
	   bool writeN(char dc, const uint8_t *data, int nbytes);
   private:
      SPIDevice *SPI;
      gpio_num_t PinDC;
      gpio_num_t PinRST;
      gpio_num_t PinBL;
      uint16_t DisplayWidth;
      uint16_t DisplayHeight;
      DISPLAY_ROTATION Rotation;
      uint8_t *SPIBuffer;
      uint16_t SPIBufferSize;
      SemaphoreHandle_t SpiSemaphoreHandle;
};

}

