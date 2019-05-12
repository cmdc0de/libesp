
#include "spibus.h"

using namespace libesp;

//statics
ErrorType SPIBus::initializeBus(spi_host_device_t shd, spi_bus_config_t &buscfg) {

}

SPIBus & SPIBus::createSPIBus(spi_host_device_t shd) {

}

ErrorType SPIBus::shutdown(spi_host_device_t shd) {

}
	
//non-statics
SPIBUS::SPIBus(spi_host_device_t shd, spi_bus_config_t &buscfg) {

}

ErrorType SPIBus::attachDevice(SPIDevice &d) {

}

ErrorType SPIBus::~SPIBus() {

}

SPIDevice SPIBus::createMasterDevice(spi_device_interface_config_t &devcfg, int dmachannel) {

}
