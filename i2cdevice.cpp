
#include "i2cdevice.h"
#include "error_type.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/i2c_master.h>
#include <esp_log.h>

using namespace libesp;

I2CDevice::I2CDevice(I2CBus *bus, const i2c_master_dev_handle_t &handle, const i2c_device_config_t &devcfg)
	: MyBus(bus), DevHandle(handle), DevCfg(devcfg) {
}

ErrorType I2CDevice::shutdown() {
	return onShutdown();
}

ErrorType I2CDevice::transmit(const uint8_t *data, size_t len, int timeout_ms) {
	return i2c_master_transmit(DevHandle, data, len, timeout_ms);
}

ErrorType I2CDevice::transmitMultiBuffer(i2c_master_transmit_multi_buffer_info_t *buffers, size_t bufferCount, int timeout_ms) {
	return i2c_master_multi_buffer_transmit(DevHandle, buffers, bufferCount, timeout_ms);
}

ErrorType I2CDevice::receive(uint8_t *data, size_t len, int timeout_ms) {
	return i2c_master_receive(DevHandle, data, len, timeout_ms);
}

ErrorType I2CDevice::transmitReceive(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len, int timeout_ms) {
	return i2c_master_transmit_receive(DevHandle, tx_data, tx_len, rx_data, rx_len, timeout_ms);
}

I2CDevice::~I2CDevice() {
}

const char *I2CMaster::LOGTAG = "I2CMASTER";

I2CMaster::I2CMaster(I2CBus *bus, const i2c_master_dev_handle_t &handle, const i2c_device_config_t &devcfg, SemaphoreHandle_t semaphore)
	: I2CDevice(bus, handle, devcfg), MySemaphore(semaphore) {
}

I2CMaster::~I2CMaster() {
	shutdown();
}

ErrorType I2CMaster::onShutdown() {
	return getBus()->removeDevice(this);
}
