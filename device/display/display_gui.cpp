#include "display_gui.h"
#include "../../freertos.h"
#include <cstring>

using namespace libesp;

void DisplayGUIListItemData::setShouldScroll() {
	if (shouldScroll()) {
		resetScrollable();
	} else {
		Scrollable = 0;
	}
}

bool DisplayGUIListItemData::shouldScroll() {
	if (strlen(text) * 6 > 120) {
		return true;
	}
	return false;
}

const char *DisplayGUIListItemData::getScrollOffset() {
	uint16_t offSet = 0;
	if (Scrollable) {
		if (LastScrollTime == 0) {
			LastScrollTime = FreeRTOS::getTimeSinceStart();//HAL_GetTick();
		}
		if (FreeRTOS::getTimeSinceStart() - LastScrollTime > TimeBetweenScroll) {
			LastScrollTime = FreeRTOS::getTimeSinceStart();//HAL_GetTick();
			LastScrollPosition++;
			uint32_t l = strlen(text);
			if (LastScrollPosition >= l) {
				LastScrollPosition = 0;
			}
		}
		offSet = LastScrollPosition;
	}
	return text + offSet;
}

void DisplayGUIListData::moveUp() {
  do {
    if (selectedItem == 0) {
      selectedItem = ItemsCount - 1;
    } else {
      selectedItem--;
    }
  } while(!items[selectedItem].text || items[selectedItem].text[0]=='\0');
}

void DisplayGUIListData::moveDown() {
  do {
    if (selectedItem == ItemsCount - 1) {
      selectedItem = 0;
    } else {
      selectedItem++;
    }
  } while(selectedItem<ItemsCount && (!items[selectedItem].text || items[selectedItem].text[0]=='\0'));
}

void DisplayGUIListData::selectTop() {
  selectedItem = 0;
}


