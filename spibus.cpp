
#include "spibus.h"
#include "spidevice.h"

using namespace libesp;

static SPIBus * ESP32SPIBuses[3] = {0};

//statics
ErrorType SPIBus::initializeBus(spi_host_device_t shd, spi_bus_config_t &buscfg, int dmachannel) {
	esp_err_t ret;
	ErrorTyper et = shutdown(shd);
	if(et.ok()) {
		ret=spi_bus_initialize(shd, &buscfg, dmachannel);
		if(ret!=ESP_OK) return ErrorType(ret);
		ESP32SPIBuses[shd] = new SPIBus(shd, buscfg, dmachannel);
		return ErrorType(ret);
	} 
	return et;
}

SPIBus *SPIBus::get(spi_host_device_t shd) {
	return ESP32SPIBuses[shd];
}

ErrorType SPIBus::shutdown(spi_host_device_t shd) {
	if(ESP32SPIBuses[shd]!=0) {
		ret = ESP32SPIBuses[shd]->shutdown();
		if(ret!=ESP_OK) return ErrorType(ret);
		ESP32SPIBuses[shd] = 0;
	}
	return ErrorType();
}
	
//non-statics
SPIBUS::SPIBus(spi_host_device_t shd, spi_bus_config_t &buscfg, int dma) :
	SPIBusID(shd), BusConfig(budcfg), DMAChannel(dma) {

}

ErrorType SPIBus::removeDevice(SPIDevice *d) {
	esp_err_t ret = spi_bus_remove_device(d->getDeviceHandle());
	return ErrorType(ret);
}


ErrorType SPIBus::~SPIBus() {
	ATTACHED_DEVICE_TYPE_IT it = AttachedDevices.begin();
	for(;it!=AttachedDevices.end();++it) {
		removeDevice((*it));
	}
	AttachedDevices.clear();
}

SPIDevice *SPIBus::createMasterDevice(spi_device_interface_config_t &devcfg) {
	spi_device_handle_t spi;
	esp_err_t ret=spi_bus_add_device(SPIBusID, &devcfg, &spi);
	if(ret==ESP_OK) {
		SPIMaster *s = new SPIMaster(this,spi_device_interface_config_t &devcfg);
		AttachedDevices.push_back(s);
	}
}
