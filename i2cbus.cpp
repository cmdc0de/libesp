
#include <driver/i2c_master.h>
#include "i2cbus.h"
#include "i2cdevice.h"
#include <algorithm>

using namespace libesp;

static I2CBus *I2CBuses[2] = {0};

ErrorType I2CBus::initializeBus(const i2c_port_t &port, const i2c_master_bus_config_t &buscfg) {
	esp_err_t ret;
	ErrorType et = shutdown(port);
	if(et.ok()) {
		i2c_master_bus_handle_t bus_handle;
		ret = i2c_new_master_bus(&buscfg, &bus_handle);
		if(ret!=ESP_OK) return ErrorType(ret);
		I2CBuses[port] = new I2CBus(port, buscfg, bus_handle);
		return ErrorType(ret);
	}
	return et;
}

I2CBus *I2CBus::get(const i2c_port_t &port) {
	return I2CBuses[port];
}

ErrorType I2CBus::shutdown(const i2c_port_t &port) {
	ErrorType et;
	if(I2CBuses[port]!=0) {
		et = I2CBuses[port]->shutdown();
		if(et.ok()) {
			I2CBuses[port] = 0;
		}
	}
	return et;
}

I2CBus::I2CBus(const i2c_port_t &port, const i2c_master_bus_config_t &buscfg, i2c_master_bus_handle_t handle) :
	PortID(port), BusHandle(handle), BusConfig(buscfg) {
}

ErrorType I2CBus::removeDevice(I2CDevice *d) {
	esp_err_t ret = i2c_master_bus_rm_device(d->getDeviceHandle());
	if(ret==ESP_OK) {
		ATTACHED_DEVICE_TYPE_IT it = std::find(AttachedDevices.begin(),AttachedDevices.end(),d);
		if(it!=AttachedDevices.end()) {
			AttachedDevices.erase(it);
		}
	}
	return ErrorType(ret);
}

ErrorType I2CBus::shutdown() {
	ATTACHED_DEVICE_TYPE_IT it = AttachedDevices.begin();
	for(;it!=AttachedDevices.end();++it) {
		removeDevice((*it));
		delete (*it);
	}
	AttachedDevices.clear();
	return i2c_del_master_bus(BusHandle);
}

I2CBus::~I2CBus() {
	shutdown();
}

I2CDevice *I2CBus::createMasterDevice(const i2c_device_config_t &devcfg) {
	return createMasterDevice(devcfg, nullptr);
}

I2CDevice *I2CBus::createMasterDevice(const i2c_device_config_t &devcfg, SemaphoreHandle_t semaphore) {
	i2c_master_dev_handle_t dev_handle;
	esp_err_t err = i2c_master_bus_add_device(BusHandle, &devcfg, &dev_handle);
	if(err==ESP_OK) {
		I2CMaster *s = new I2CMaster(this, dev_handle, devcfg, semaphore);
		AttachedDevices.push_back(s);
		return s;
	}
	return nullptr;
}
