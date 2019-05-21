
#ifndef LIBESP_DEVICE_H
#define LIBESP_DEVICE_H

#include "spibus.h"

namespace libesp {


class SPIDevice {
public:
	bool init();
	ErrorType shutdown(); 
	ErrorType sendAndReceive(uint8_t out, uint8_t &in);
	ErrorType sendAndReceive(uint8_t *p, uint16_t len);
	ErrorType send(uint8_t *p, uint16_t len);
	virtual ~SPIDevice();
	SPIBus *getBus() {return MyBus;}
	const spi_device_interface_config_t &getInterfaceConfig() const {return DevCfg;}
	const spi_device_handle_t &getDeviceHandle() const {return SPIHandle;}
protected:
	SPIDevice(SPIBus* bus, const spi_device_handle_t &s, const spi_device_interface_config_t &devcfg);
	virtual bool onInit()=0;
	virtual ErrorType onShutdown()=0;
	virtual ErrorType onSendAndReceive(uint8_t out, uint8_t &in)=0;
	virtual ErrorType onSendAndReceive(uint8_t *p, uint16_t len)=0;
	virtual ErrorType onSend(uint8_t *p, uint16_t len)=0;
protected:
	SPIBus *MyBus;
	spi_device_handle_t SPIHandle;
	spi_device_interface_config_t DevCfg;
};

/////////
//
class SPIMaster : public SPIDevice {
public:
	virtual ~SPIMaster();
public:
protected:
	ErrorType onShutdown(); 
	SPIMaster(SPIBus*b, const spi_device_handle_t &s, const spi_device_interface_config_t &devcfg);
	virtual ErrorType onSendAndReceive(uint8_t out, uint8_t &in);
	virtual ErrorType onSendAndReceive(uint8_t *p, uint16_t len);
	virtual ErrorType onSend(uint8_t *p, uint16_t len);
	virtual bool onInit();
private:
	friend class SPIBus;
};

}
#endif
