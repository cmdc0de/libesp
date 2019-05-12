
#ifndef LIBESP_WIRING_H
#define LIBESP_WIRING_H

namespace libesp {

class SPIDevice {
public:
	SPIDevice();
	bool init();
	ErrorType shutdown(); 
	virtual ErrorType sendAndReceive(uint8_t out, uint8_t &in)=0;
	virtual ErrorType send(uint8_t *p, uint16_t len) =0;
	virtual ErrorType onShutdown()=0;
	virtual ~SPIDevice();
protected:
	virtual bool onInit()=0;
	virtual ErrorType onShutdown();
};

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
	SPIDevice &createMasterDevice(spi_device_interface_config_t &devcfg, int dmachannel);
protected:
	SPIBus(spi_host_device_t shd, spi_bus_config_t &buscfg);
private:
	spi_host_device_t SPIBusID;
	spi_bus_config_t BusConfig;
};
/////////
//
class SPIMaster : public SPIDevice {
public:
	static ESP32SPIWiring create(SPIBus &bus,spi_device_interface_config_t &devcfg, int dmachannel);
public:
	virtual ErrorType sendAndReceive(uint8_t out, uint8_t &in);
	virtual ErrorType send(uint8_t *p, uint16_t len);
	virtual ~ESP32SPIWiring();
public:
	ErrorType onShutdown(); 
protected:
	ESP32SPIWiring(spi_device_interface_config_t &devcfg, int dmachannel);
	virtual bool onInit();
private:
	spi_host_device_t SpiHD;
	spi_bus_config_t BusCfg;
	spi_device_interface_config_t DevCfg;
	int DMAChannel;
	spi_device_handle_t spi;
};
}
#endif
