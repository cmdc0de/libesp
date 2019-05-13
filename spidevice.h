
#ifndef LIBESP_WIRING_H
#define LIBESP_WIRING_H

namespace libesp {

class SPIBus;

class SPIDevice {
public:
	SPIDevice(SPIBus* bus);
	bool init();
	ErrorType shutdown(); 
	virtual ErrorType sendAndReceive(uint8_t out, uint8_t &in)=0;
	virtual ErrorType send(uint8_t *p, uint16_t len) =0;
	virtual ErrorType onShutdown()=0;
	virtual ~SPIDevice();
	SPIBus *getBus() {return MyBus;}
protected:
	virtual bool onInit()=0;
	virtual ErrorType onShutdown();
	SPIBus *MyBus;
};

/////////
//
class SPIMaster : public SPIDevice {
public:
	virtual ErrorType sendAndReceive(uint8_t out, uint8_t &in);
	virtual ErrorType send(uint8_t *p, uint16_t len);
	virtual ~ESP32SPIWiring();
public:
protected:
	ErrorType onShutdown(); 
	SPIMaster(SPIBus*b, spi_device_handle_t s, spi_device_interface_config_t &devcfg);
	virtual bool onInit();
private:
	spi_device_interface_config_t DevCfg;
	spi_device_handle_t spi;
	friend class SPIBus;
};
}
#endif
