
#ifndef LIBESP_WIRING_H
#define LIBESP_WIRING_H

#include <vector>
#include "error_type.h"
#include <driver/spi_master.h>

namespace libesp {

class SPIDevice;

// ESP32 SPI BUss
class SPIBus {
public:
	typedef std::vector<SPIDevice*> ATTACHED_DEVICE_TYPE;
	typedef ATTACHED_DEVICE_TYPE::iterator ATTACHED_DEVICE_TYPE_IT;
public: 
	static ErrorType initializeBus(const spi_host_device_t &shd, const spi_bus_config_t &buscfg, int dmaChan);
	static SPIBus *get(const spi_host_device_t &shd);
	static ErrorType shutdown(const spi_host_device_t &shd);
public:	
	~SPIBus();
	const spi_host_device_t &getSPIBusID() const {return SPIBusID;}
	const spi_bus_config_t &getSPIBusConfig() const {return BusConfig;}
	int getDMAChannel() const {return DMAChannel;} 
	/*
	* create a master device for this SPI Bus
	*/
	SPIDevice *createMasterDevice(const spi_device_interface_config_t &devcfg);
	/*
	* removes the device from this Bus, SPI device itself is not destroyed, device is not destoryed
	*/
	ErrorType removeDevice(SPIDevice *d);
	/*
	* remove all devices, destroy all devices, free spi bus
	*/
	ErrorType shutdown();
protected:
	SPIBus(const spi_host_device_t &shd, const spi_bus_config_t &buscfg, int dma);
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
