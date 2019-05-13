
#ifndef LIBESP_WIRING_H
#define LIBESP_WIRING_H

#include <vector>

namespace libesp {

class SPIDevice;

// ESP32 SPI BUss
class SPIBus {
public:
	typedef std::vector<SPIDevice*> ATTACHED_DEVICE_TYPE;
	typedef ATTACHED_DEVICE_TYPE::iterator ATTACHED_DEVICE_TYPE_IT;
public: 
	static ErrorType initializeBus(spi_host_device_t shd, spi_bus_config_t &buscfg, int dmaChan);
	static SPIBus & get(spi_host_device_t shd);
	static ErrorType shutdown(spi_host_device_t shd);
public:	
	~SPIBus();
	const spi_host_device_t &getSPIBusID() const {return SPIBusID;}
	const spi_bus_config_t &getSPIBusConfig() const {return BusConfig;}
	SPIDevice *createMasterDevice(spi_device_interface_config_t &devcfg);
	ErrorType removeDevice(SPIDevice *d);
protected:
	SPIBus(spi_host_device_t shd, spi_bus_config_t &buscfg);
private:
	//break copy ctor
	SPIBus(const SPIBus &r);
private:
	spi_host_device_t SPIBusID;
	spi_bus_config_t BusConfig;
	int DMAChannel;
	std::vector<SPIDevice*> AttachedDevices;
};
}
#endif
