
#ifndef LIBESP_I2CBUS_H
#define LIBESP_I2CBUS_H

#include <etl/vector.h>
#include "error_type.h"
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace libesp {

class I2CDevice;

class I2CBus {
public:
	typedef etl::vector<I2CDevice*,4> ATTACHED_DEVICE_TYPE;
	typedef ATTACHED_DEVICE_TYPE::iterator ATTACHED_DEVICE_TYPE_IT;
public:
	static ErrorType initializeBus(const i2c_port_t &port, const i2c_master_bus_config_t &buscfg);
	static I2CBus *get(const i2c_port_t &port);
	static ErrorType shutdown(const i2c_port_t &port);
public:
	~I2CBus();
	const i2c_port_t &getPortID() const {return PortID;}
	const i2c_master_bus_handle_t &getBusHandle() const {return BusHandle;}
	const i2c_master_bus_config_t &getBusConfig() const {return BusConfig;}
	I2CDevice *createMasterDevice(const i2c_device_config_t &devcfg);
	I2CDevice *createMasterDevice(const i2c_device_config_t &devcfg, SemaphoreHandle_t semaphore);
	ErrorType removeDevice(I2CDevice *d);
	ErrorType shutdown();
protected:
	I2CBus(const i2c_port_t &port, const i2c_master_bus_config_t &buscfg, i2c_master_bus_handle_t handle);
private:
	I2CBus(const I2CBus &r);
private:
	i2c_port_t PortID;
	i2c_master_bus_handle_t BusHandle;
	i2c_master_bus_config_t BusConfig;
	ATTACHED_DEVICE_TYPE AttachedDevices;
};
}
#endif
