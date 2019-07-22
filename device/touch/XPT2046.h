#ifndef LIBESP_TOUCH_XPT2046_H
#define  LIBESP_TOUCH_XPT2046_H

#include <set>
#include <libesp/error_type.h>
#include <libesp/spidevice.h>
#include <libesp/task.h>
#include <atomic>

namespace libesp {
	
class TouchNotification {
public:
	TouchNotification(int16_t x, int16_t y, int16_t z, bool down) 
	: XPos(x), YPos(y), ZPos(z), PenDown(down) {}
	int16_t getX() const {return XPos;}
	int16_t getY() const {return YPos;}
	int16_t getZ() const {return ZPos;}
	bool isPenDown() const {return PenDown;}
private:
	int16_t XPos;
	int16_t YPos;
	int16_t ZPos;
	bool PenDown;
};

class XPT2046 : public Task {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
public:
	struct PenEvent {
		static const uint8_t PEN_EVENT_DOWN=1;
		static const uint8_t PEN_SET_PWR_MODE=2;
		PenEvent(uint8_t e) : EvtType(e) {}
		uint8_t EvtType;
	};
	static PenEvent PenDownEvt;
	static PenEvent PenPwrModeEvnt;
	static const int STARTBIT			= 0b10000000;
	static const int ACQUIRE_MASK		= 0b01110000;
	static const int YPOS				= 0b00010000;
	static const int Z1POS				= 0b00110000;
	static const int Z2POS				= 0b01000000;
	static const int XPOS				= 0b01010000;
	static const int MODE_MASK			= 0b00001000;
	static const int BIT_MODE_12		= 0b00000000;
	static const int BIT_MODE_8		= 0b00001000;
	static const int SER_DEF_MASK		= 0b00000100;
	static const int SER					= 0b00000100;
	static const int DFR					= 0b00000000; //recommended Page 17 of data sheet
	static const int PWR_DOWN_MASK	= 0b00000011;
	static const int PWR_DOWN_BTW		= 0b00000000; // Power down between conversations. When each converssation is finished the conversation will enter lowpower mode
	static const int REF_OFF_ADC_ON	= 0b00000001; //Pen IRQ disabled
	static const int REF_ON_ADC_OFF	= 0b00000010;
	static const int REF_OFF_ADC_OFF = 0b00000011; // PEN IRQ DISABLED
	struct ControlByte {
		union {
			uint8_t c;
			struct {
				uint8_t StartBit:1;
				uint8_t AcquireBits:3;
				uint8_t ModeBit:1;
				uint8_t SerDFR:1;
				uint8_t PwrMode:2;
			};
		};
	};
public:
	static ErrorType initTouch(gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, spi_host_device_t spiNum, int channel);
public:
	XPT2046(gpio_num_t interruptPin, bool swapXY);
	
	ErrorType init(SPIBus *bus, gpio_num_t cs);
	//if pen irq fires ShouldProcess will be set
	// until pen comes off (another interrupt) we will meausre pen position
	// each position measured will measured, MeasurementsToAverage times, with a 'time between mesaures' as well.
	virtual void run(void *data);
	/*
	* add / remove observers
	*/
	bool addObserver(const xQueueHandle &o);
	bool removeObserver(const xQueueHandle &o);
	/*
	* does not try to acquire just sets pwr mode
	* if the power mode turns off the pen the acquision will stay off
	*/
	void setPwrMode(uint8_t pwrMode);
	QueueHandle_t getInternalQueueHandle() {return InternalQueueHandler;}
	~XPT2046();
	uint32_t getPenX() {return PenX;}
	uint32_t getPenY() {return PenY;}
	uint32_t getPenZ() {return PenZ;}
	bool isPenDown() {return IsPenDown;}
	void broadcast(); 
public:
	static const int TOUCH_QUEUE_SIZE = 4;
	static const int TOUCH_MSG_SIZE = sizeof(PenEvent*);
	static const char *LOGTAG;
protected:
	//set up irq: one for neg edge pen down
	virtual void onStart();
	virtual void onStop();
private:
	std::set<xQueueHandle> Notifications;
	SPIDevice *MyDevice;
	gpio_num_t InterruptPin;
	QueueHandle_t InternalQueueHandler;
	ControlByte MyControlByte;
	volatile int32_t PenX;
	volatile int32_t PenY;
	volatile int32_t PenZ;
	volatile std::atomic<bool> IsPenDown;
	bool SwapXY;
	QueueHandle_t BroadcastQueueHandler;
};

}

#endif
