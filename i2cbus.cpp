
#include <driver/spi_common.h>
#include "spibus.h"
#include "spidevice.h"
#include <algorithm>

using namespace libesp;

static I2CBus * I2CBuses[3] = {0};

//statics
ErrorType I2cBus::initializeBus(const i2c_port_t &shd, const i2c_master_bus_config_t &buscfg) {
	esp_err_t ret;
	ErrorType et = shutdown(shd);
	if(et.ok()) {
		i2c_master_bus_handle_t bus_handle;
		ret = i2c_new_master_bus(&bus_config, &bus_handle);
		if(ret!=ESP_OK) return ErrorType(ret);
		I2CBuses[shd] = new I2CBus(shd, buscfg);
		return ErrorType(ret);
	} 
	return et;
}

I2CBus *I2CBus::get(const spi_host_device_t &shd) {
	return I2CBuses[shd];
}

ErrorType I2CBus::shutdown(const spi_host_device_t &shd) {
	ErrorType et;
	if(I2CBuses[shd]!=0) {
		et = I2CBuses[shd]->shutdown();
		if(et.ok()) {
			I2CBuses[shd] = 0;
		}
	}
	return et;
}
	
//non-statics
	
I2CBus::I2CBus(const i2c_port_t &shd, const i2c_master_bus_config_t &buscfg) :
	I2CBusID(shd), BusConfig(buscfg) {

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
	return i2c_del_master_bus(I2CBusID);
}

I2CBus::~I2CBus() {
	shutdown();
}

I2CDevice *I2CBus::createMasterDevice(const i2c_device_config_t &devcfg) {
  return createMasterDevice(devcfg,nullptr);
}

I2CDevice *I2CBus::createMasterDevice(const i2c_device_config_t &devcfg, SemaphoreHandle_t spiSemaphore) {
	i2c_master_dev_handle_t dev_handle;
	esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
	if(ret==ESP_OK) {
		I2CMaster *s = new I2CMaster(this,spi,devcfg, spiSemaphore);
		AttachedDevices.push_back(s);
		return s;
	}
	return nullptr;
}
