
#ifndef LIBESP_WIRING_H
#define LIBESP_WIRING_H

class Wiring {
public:
	Wiring();
	bool init();
	ErrorType shutdown(); 
	virtual ErrorType sendAndReceive(uint8_t out, uint8_t &in)=0;
	virtual ErrorType send(uint8_t *p, uint16_t len) =0;
	virtual ErrorType onShutdown()=0;
	virtual ~Wiring();
protected:
	virtual bool onInit()=0;
};


/////////
class ESP32SPIWiring : public Wiring {
public:
	static ESP32SPIWiring create(spi_host_device_t shd, int miso, int mosi, int clk, int cs, int tb, int dma);
public:
	virtual ErrorType sendAndReceive(uint8_t out, uint8_t &in);
	virtual ErrorType send(uint8_t *p, uint16_t len);
	virtual ~ESP32SPIWiring();
public:
	ErrorType onShutdown(); 
protected:
	ESP32SPIWiring(spi_host_device_t spihd, int miso, int mosi, int clk, int cs, int bufSize, int dmachannel);
	virtual bool onInit();
private:
	spi_host_device_t SpiHD;
	int PinMiso;
	int PinMosi;
	int PinCLK;
	int PinCS;
	int TransferBufferSize;
	int DMAChannel;
	spi_device_handle_t spi;
};

#endif
