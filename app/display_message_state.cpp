#include "display_message_state.h"
#include "../device/display/display_device.h"
#include <cstring>

using namespace libesp;

//=======================================================================
DisplayMessageState::DisplayMessageState() :
		TimeInState(DEFAULT_TIME_IN_STATE), NextState(0), Display(0) {
}

DisplayMessageState::~DisplayMessageState() {
}

ErrorType DisplayMessageState::onInit() {
	getDisplay().fillScreen(RGBColor::BLACK);
	return ErrorType();
}

void DisplayMessageState::setMessage(const char *msg) {
	strncpy(&this->Message[0], msg, sizeof(this->Message));
}

BaseMenu::ReturnStateContext DisplayMessageState::onRun() {
	getDisplay().drawString(0, 10, &this->Message[0], RGBColor::WHITE, RGBColor::BLACK, 1, true);
	if (timeInState() > TimeInState) { 
		return ReturnStateContext(getNextState());
	}
	return ReturnStateContext(this);
}

ErrorType DisplayMessageState::onShutdown() {
	return ErrorType();
}
