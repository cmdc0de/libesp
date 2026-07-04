
#ifndef LIBESP_I2CDEVICE_H
#define LIBESP_I2CDEVICE_H

#include "i2cbus.h"

namespace libesp {

class I2CDevice {
public:
	ErrorType shutdown();
	ErrorType transmit(const uint8_t *data, size_t len, int timeout_ms);
	ErrorType transmitMultiBuffer(i2c_master_transmit_multi_buffer_info_t *buffers, size_t bufferCount, int timeout_ms);
	ErrorType receive(uint8_t *data, size_t len, int timeout_ms);
	ErrorType transmitReceive(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len, int timeout_ms);
	virtual ~I2CDevice();
	I2CBus *getBus() {return MyBus;}
	const i2c_device_config_t &getDeviceConfig() const {return DevCfg;}
	const i2c_master_dev_handle_t &getDeviceHandle() const {return DevHandle;}
protected:
	I2CDevice(I2CBus *bus, const i2c_master_dev_handle_t &handle, const i2c_device_config_t &devcfg);
	virtual ErrorType onShutdown() = 0;
protected:
	I2CBus *MyBus;
	i2c_master_dev_handle_t DevHandle;
	i2c_device_config_t DevCfg;
};

class I2CMaster : public I2CDevice {
public:
	virtual ~I2CMaster();
	static const char *LOGTAG;
	static const uint32_t MILLIS_DEFAULT_WAIT = 5;
protected:
	I2CMaster(I2CBus *bus, const i2c_master_dev_handle_t &handle, const i2c_device_config_t &devcfg, SemaphoreHandle_t semaphore);
	virtual ErrorType onShutdown();
private:
	SemaphoreHandle_t MySemaphore;
	friend class I2CBus;
};

}
#endif
