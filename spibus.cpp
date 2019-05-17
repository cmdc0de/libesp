
#include <driver/spi_common.h>
#include "spibus.h"
#include "spidevice.h"
#include <algorithm>

using namespace libesp;

static SPIBus * ESP32SPIBuses[3] = {0};

//statics
ErrorType SPIBus::initializeBus(const spi_host_device_t &shd, const spi_bus_config_t &buscfg, int dmachannel) {
	esp_err_t ret;
	ErrorType et = shutdown(shd);
	if(et.ok()) {
		ret=::spi_bus_initialize(shd, &buscfg, dmachannel);
		if(ret!=ESP_OK) return ErrorType(ret);
		ESP32SPIBuses[shd] = new SPIBus(shd, buscfg, dmachannel);
		return ErrorType(ret);
	} 
	return et;
}

SPIBus *SPIBus::get(const spi_host_device_t &shd) {
	return ESP32SPIBuses[shd];
}

ErrorType SPIBus::shutdown(const spi_host_device_t &shd) {
	ErrorType et;
	if(ESP32SPIBuses[shd]!=0) {
		et = ESP32SPIBuses[shd]->shutdown();
		if(et.ok()) {
			ESP32SPIBuses[shd] = 0;
		}
	}
	return et;
}
	
//non-statics
SPIBus::SPIBus(const spi_host_device_t &shd, const spi_bus_config_t &buscfg, int dma) :
	SPIBusID(shd), BusConfig(buscfg), DMAChannel(dma) {

}

ErrorType SPIBus::removeDevice(SPIDevice *d) {
	esp_err_t ret = spi_bus_remove_device(d->getDeviceHandle());
	if(ret==ESP_OK) {
		ATTACHED_DEVICE_TYPE_IT it = std::find(AttachedDevices.begin(),AttachedDevices.end(),d);
		if(it!=AttachedDevices.end()) {
			AttachedDevices.erase(it);
		}
	}
	return ErrorType(ret);
}

ErrorType SPIBus::shutdown() {
	ATTACHED_DEVICE_TYPE_IT it = AttachedDevices.begin();
	for(;it!=AttachedDevices.end();++it) {
		removeDevice((*it));
		delete (*it);
	}
	AttachedDevices.clear();
	return spi_bus_free(SPIBusID);
}

SPIBus::~SPIBus() {
	shutdown();
}

SPIDevice *SPIBus::createMasterDevice(const spi_device_interface_config_t &devcfg) {
	spi_device_handle_t spi;
	esp_err_t ret=spi_bus_add_device(SPIBusID, &devcfg, &spi);
	if(ret==ESP_OK) {
		SPIMaster *s = new SPIMaster(this,spi,devcfg);
		AttachedDevices.push_back(s);
		return s;
	}
	return nullptr;
}
