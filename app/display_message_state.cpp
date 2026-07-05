#include "display_message_state.h"
#include "../device/display/display.h"
#include <cstring>

using namespace libesp;

IDisplayMessageDisplay::~IDisplayMessageDisplay() {}

//=======================================================================
DisplayMessageState::DisplayMessageState() :
		TimeInState(DEFAULT_TIME_IN_STATE), NextState(0), DisplayAdapter(nullptr)
		, MsgX(30), MsgY(100) {
}

DisplayMessageState::~DisplayMessageState() {
}

ErrorType DisplayMessageState::onInit() {
	DisplayAdapter->clearScreen();
	return ErrorType();
}

void DisplayMessageState::setMessage(const char *msg) {
	strncpy(&this->Message[0], msg, sizeof(this->Message));
	this->Message[sizeof(this->Message)-1] = '\0';
}

BaseMenu::ReturnStateContext DisplayMessageState::onRun() {
	DisplayAdapter->drawString(MsgX, MsgY, &this->Message[0], RGBColor::WHITE, RGBColor::BLACK, 1, true);
	if (timeInState() > TimeInState) { 
		return ReturnStateContext(getNextState());
	}
	return ReturnStateContext(this);
}

ErrorType DisplayMessageState::onShutdown() {
	return ErrorType();
}
