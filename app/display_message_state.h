#ifndef CMDC0DE_DISPLAY_MESSAGE_STATE_H
#define CMDC0DE_DISPLAY_MESSAGE_STATE_H

#include "basemenu.h"

namespace libesp {

class IDisplay;

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
	void setDisplay(IDisplay *dd) {Display = dd;}
	IDisplay *getDisplay() {return Display;}
	const BaseMenu *getNextState() const {return NextState;}
protected:
	virtual ErrorType onInit();
	virtual ReturnStateContext onRun();
	virtual ErrorType onShutdown();
private:
	char Message[64];
	uint16_t TimeInState;
	BaseMenu *NextState;
	IDisplay *Display;
};

}

#endif
