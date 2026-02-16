#include "display_message_state.h"
#include "../device/display/display.h"
#include <cstring>

using namespace libesp;

IDisplayMessageDisplay::~IDisplayMessageDisplay() {}

//=======================================================================
DisplayMessageState::DisplayMessageState() :
		TimeInState(DEFAULT_TIME_IN_STATE), NextState(0), DisplayAdapter(nullptr) {
}

DisplayMessageState::~DisplayMessageState() {
}

ErrorType DisplayMessageState::onInit() {
	DisplayAdapter->clearScreen();
	return ErrorType();
}

void DisplayMessageState::setMessage(const char *msg) {
	strncpy(&this->Message[0], msg, sizeof(this->Message));
}

BaseMenu::ReturnStateContext DisplayMessageState::onRun() {
	DisplayAdapter->drawString(30, 100, &this->Message[0], RGBColor::WHITE, RGBColor::BLACK, 1, true);
	if (timeInState() > TimeInState) { 
		return ReturnStateContext(getNextState());
	}
	return ReturnStateContext(this);
}

ErrorType DisplayMessageState::onShutdown() {
	return ErrorType();
}
