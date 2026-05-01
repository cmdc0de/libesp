
#ifndef LIBESP_WIRING_H
#define LIBESP_WIRING_H

#include <etl/vector.h>
#include "error_type.h"
#include <driver/i2c_master.h>
#include <freertos/semphr.h>

namespace libesp {

class I2CDevice;

// ESP32 i2C BUss
class I2CBus {
public:
	typedef etl::vector<I2CDevice*,4> ATTACHED_DEVICE_TYPE;
	typedef ATTACHED_DEVICE_TYPE::iterator ATTACHED_DEVICE_TYPE_IT;
public: 
	static ErrorType initializeBus(const i2c_port_t &shd, const i2c_master_bus_config_t &buscfg);
	static SPIBus *get(const i2c_port_t &shd);
	static ErrorType shutdown(const i2c_port_t &shd);
public:	
	~SPIBus();
	const i2c_port_t &getSPIBusID() const {return SPIBusID;}
	const i2c_master_bus_config_t &getSPIBusConfig() const {return BusConfig;}
	/*
	* create a master device for this SPI Bus
	*/
	SPIDevice *createMasterDevice(const i2c_device_config_t &devcfg);
	SPIDevice *createMasterDevice(const i2c_device_config_t &devcfg, SemaphoreHandle_t spiSemaphore);
	/*
	* removes the device from this Bus, SPI device itself is not destroyed, device is not destoryed
	*/
	ErrorType removeDevice(I2CDevice *d);
	/*
	* remove all devices, destroy all devices, free spi bus
	*/
	ErrorType shutdown();
protected:
	I2CBus(const i2c_port_t &shd, const i2c_master_bus_config_t &buscfg);
private:
	//break copy ctor
	I2CBus(const I2CBus &r);
private:
	i2c_port_t SPIBusID;
	i2c_master_bus_config_t BusConfig;
	ATTACHED_DEVICE_TYPE AttachedDevices;
};
}
#endif
