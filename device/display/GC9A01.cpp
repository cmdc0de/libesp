
#include "GC9A01.h"
#include "error_type.h"
#include "spibus.h"

using namespace libesp;


GC9A01::GC901(DISPLAY_ROTATION r, uint8_t *spiBuffer, uint16_t bufferSize) 
      : SPI(0), PinDC(GPIO_NUM_NC), PinRST(GPIO_NUM_NC), DisplayWidth(240), DisplayHeight(240)
        , DisplayRotation(r), SPIBuffer(spiBuffer), SPIBufferSize(bufferSize), SpiSemaphoreHandle(0) {

}

GC9A01::~GC901() {

}

ErrorType GC9A01::init() {
   return ErrorType();
}

ErrorType GC9A01::createInitDevice(SPIBus *bus, gpio_num_t cs, gpio_num_t data_cmd) {
   return ErrorType();
}

ErrorType GC9A01::createInitDevice(SPIBus *bus, gpio_num_t cs, gpio_num_t data_cmd, SemaphoreHandle_t handle) {
   SpiSemaphoreHandle = handle;
   PinDC = data_cmd;
   SPIBus = bus;
   return ErrorType();
}

ErrorType GC9A01::setRotation(DISPLAY_ROTATION rotation) {
   return ErrorType();
}

ErrorType GC9A01::sendData(const uint8_t *data, uint16_t len) {
   return ErrorType();
}

ErrorType GC9A01::backlight(uint8_t level) {
   return ErrorType();
}

ErrorType GC9A01::reset() {
   return ErrorType();
}

void GC9A01::setPixelFormat(BasicBackBuffer *backBuff) {

}

ErrorType GC9A01::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
   return ErrorType();
}

ErrorType GC9A01::swap(BasicBackBuffer *backBuffer) {
   return ErrorType();
}

bool GC9A01::writeCmd(uint8_t c) {
   return false;
}

bool GC9A01::writeNData(const uint8_t *data, int nbytes) {
   return false;
}

bool GC9A01::write16Data(const uint16_t &data) {
   return false;
}

bool GC9A01::writeN(char dc, const uint8_t *data, int nbytes) {
   return false;
}

