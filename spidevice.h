
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
	ErrorType sendAndReceive(uint8_t *out, uint8_t *in, uint16_t len);
	ErrorType sendAndReceive(uint8_t *out, uint8_t *in, uint16_t len, void *userData);
	ErrorType send(const uint8_t *p, uint16_t len);
	ErrorType send(const uint8_t *p, uint16_t len, void *userData);
	ErrorType receive(uint8_t *p, uint16_t len, void *userData);
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
	virtual ErrorType onSendAndReceive(uint8_t *out, uint8_t *in, uint16_t len, void *userData)=0;
	virtual ErrorType onSend(const uint8_t *p, uint16_t len, void *userData)=0;
	virtual ErrorType onReceive(uint8_t *p, uint16_t len, void *userData)=0;
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
	static const char *LOGTAG;
  static const uint32_t MILLIS_DEFAULT_WAIT = 5;
protected:
	ErrorType onShutdown(); 
	SPIMaster(SPIBus*b, const spi_device_handle_t &s, const spi_device_interface_config_t &devcfg, SemaphoreHandle_t sema);
	virtual ErrorType onSendAndReceive(uint8_t out, uint8_t &in);
	virtual ErrorType onSendAndReceive(uint8_t *p, uint16_t len);
	virtual ErrorType onSendAndReceive(uint8_t *out, uint8_t *in, uint16_t len, void *userData);
	virtual ErrorType onSend(const uint8_t *p, uint16_t len, void *userData);
	virtual ErrorType onReceive(uint8_t *p, uint16_t len, void *userData);
	virtual bool onInit();
  void setMillsSemaphoreWait(uint32_t t) {MillisToWait = t;}
private:
  SemaphoreHandle_t MySemaphore;
  uint32_t MillisToWait;
	friend class SPIBus;
};

}
#endif
