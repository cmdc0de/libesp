#ifndef CMDC0DE_DISPLAY_MESSAGE_STATE_H
#define CMDC0DE_DISPLAY_MESSAGE_STATE_H

#include "basemenu.h"
#include "../device/display/color.h"

namespace libesp {

//minimal display needs adaptered as its in a library
class IDisplayMessageDisplay {
public:
	IDisplayMessageDisplay() {}	
	virtual void clearScreen()=0;
	virtual void drawString(uint16_t xPos, uint16_t yPos, const char *msg, const RGBColor &t, const RGBColor &b, uint16_t sizeMultiplier, bool wrapMessage)=0;
	virtual ~IDisplayMessageDisplay()=0;
};

class DisplayMessageState: public BaseMenu {
public:
	static const uint16_t DEFAULT_TIME_IN_STATE = 3000;
public:
	DisplayMessageState();
	virtual ~DisplayMessageState();
	void setMessage(const char *msg);
	void setTimeInState(uint16_t t) {
		TimeInState = t;
	}
	void setNextState(BaseMenu *b) {
		NextState = b;
	}
	BaseMenu *getNextState() {
		return NextState;
	}
	void setDisplay(IDisplayMessageDisplay *dd) {DisplayAdapter = dd;}
	//where the (wrapping) message is drawn; default matches the historical 240x240 layout
	void setMessagePosition(uint16_t x, uint16_t y) {MsgX = x; MsgY = y;}
	const BaseMenu *getNextState() const {return NextState;}
protected:
	virtual ErrorType onInit();
	virtual ReturnStateContext onRun();
	virtual ErrorType onShutdown();
private:
	char Message[64];
	uint16_t TimeInState;
	BaseMenu *NextState;
	IDisplayMessageDisplay *DisplayAdapter;
	uint16_t MsgX;
	uint16_t MsgY;
};

}

#endif
