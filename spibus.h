
#ifndef LIBESP_WIRING_H
#define LIBESP_WIRING_H

namespace libesp {

// ESP32 SPI BUss
class SPIBus {
public: 
	static ErrorType initializeBus(spi_host_device_t shd, spi_bus_config_t &buscfg);
	static SPIBus & createSPIBus(spi_host_device_t shd);
	static ErrorType shutdown(spi_host_device_t shd);
public:	
	ErrorType attachDevice(SPIDevice &d);
	~SPIBus();
	const spi_host_device_t &getSPIBusID() const {return SPIBusID;}
	const spi_bus_config_t &getSPIBusConfig() const {return BusConfig;}
	SPIDevice createMasterDevice(spi_device_interface_config_t &devcfg, int dmachannel);
protected:
	SPIBus(spi_host_device_t shd, spi_bus_config_t &buscfg);
private:
	spi_host_device_t SPIBusID;
	spi_bus_config_t BusConfig;
};
}
#endif
