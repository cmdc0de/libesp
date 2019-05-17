#ifndef LIBESP_TOUCH_XPT2046_H
#define  LIBESP_TOUCH_XPT2046_H

#include <set>
#include <libesp/error_type.h>
#include <libesp/spidevice.h>
#include <stdatomic.h>

namespace libesp {

class XPT2046 {
public:
	class Notification {
	public:
		Notification(uint16_t x, uint16_t y) : XPos(x), YPos(y) {}
	private:
		uint16_t XPos;
		uint16_t YPos;
	};
public:
	static const int STARTBIT			= 0b10000000;
	static const int ACQUIRE_MASK		= 0b01110000;
	static const int YPOS				= 0b00010000;
	static const int Z1POS				= 0b00110000;
	static const int Z2POS				= 0b01000000;
	static const int XPOS				= 0b01010000;
	static const int MODE_MASK			= 0b00001000;
	static const int 12BIT_MODE		= 0b00000000;
	static const int 8BIT_MODE			= 0b00001000;
	static const int SER_DEF_MASK		= 0b00000100;
	static const int SER					= 0b00000100;
	static const int DFR					= 0b00000000; //recommended Page 17 of data sheet
	static const int PWR_DOWN_MASK	= 0b00000011;
	static const int PWR_DOWN_BTW		= 0b00000000; // Power down between conversations. When each converssation is finished the conversation will enter lowpower mode
	static const int REF_OFF_ADC_ON	= 0b00000001; //Pen IRQ disabled
	static const int REF_ON_ADC_OFF	= 0b00000010;
	static const int REF_OFF_ADC_OFF = 0b00000011; // PEN IRQ DISABLED
public:
	XPT2046(SPIDevice *device, uint32_t measurementsToAverage, int32_t msBetweenMeaures);
	//set up irq -
	// one for pos edge pen down
	// one for neg edge pen up
	ErrorType init(const gpio_num_t &interruptPin);
	//if pen irq fires ShouldProcess will be set
	// until pen comes off (another interrupt) we will meausre pen position
	// each position measured will measured, MeasurementsToAverage times, with a 'time between mesaures' as well.
	ErrorType process();
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
	void setMeasurementsToAverage(uint32_t m) {MeasurementsToAverage = m;}
	uint32_t getMesarementsToAverage() const {return MeasurementsToAverage;}
	// <0 means contineous
	void setMSBetweenMeasurements(int32_t m) { MSBetweenMeasurements = m;}
	int32_t getMSBetweenMeasurements() const {return MSBetweenMeasurements;}
	~XPT2046();
private:
	std::set<xQueueHandle> Notifications;
	SPIDevice *MyDevice;
	atomic_bool ShouldProcess;
	uint32_t MeasurementsToAverage;
	int32_t MSBetweenMeasurements;
};

}

#endif
