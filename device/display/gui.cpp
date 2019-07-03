#include "gui.h"
#include "../../freertos.h"

using namespace libesp;

void GUIListItemData::setShouldScroll() {
	if (shouldScroll()) {
		resetScrollable();
	} else {
		Scrollable = 0;
	}
}

bool GUIListItemData::shouldScroll() {
	if (strlen(text) * 6 > 120) {
		return true;
	}
	return false;
}

GUITickerData::GUITickerData(const char * txt, uint8_t X, uint8_t Y,
		uint8_t W, uint8_t H) :
		text(txt), x(X), y(Y), w(W), h(H), BorderSize(1), FontScalar(1), bg(
				RGBColor::BLACK), TextColor(RGBColor::WHITE), startTick(0) {
}

GUI::GUI(DisplayDevice *display) :
		Display(display) {

}

bool GUI::init() {
	return true;
}

void GUI::drawTicker(GUITickerData *dt) {
	uint8_t maxlen = (dt->w - dt->BorderSize * 2)
			/ Display->getFont()->FontWidth, len = 0;
	int shift = 0;
	while (*(dt->text + len) != 0)
		len++;
	if (dt->startTick == 0) {
		dt->startTick = FreeRTOS::getTimeSinceStart();
	}
	shift = ((FreeRTOS::getTimeSinceStart() - dt->startTick) / GUI_TickerSpeed) - maxlen / 2; //start delay

	if (shift > len - maxlen) {
		if (shift > len - maxlen + GUI_TickerEndDelay) {
			shift = 0;
			dt->startTick = FreeRTOS::getTimeSinceStart();//HAL_GetTick();
		} else
			shift = len - maxlen;
	}
	if (shift < 0) {
		shift = 0;
	}
	Display->drawString(dt->x, dt->y, dt->text + shift, dt->TextColor, dt->bg,
			dt->FontScalar, false);
}

const char *GUIListItemData::getScrollOffset() {
	uint16_t offSet = 0;
	if (Scrollable) {
		if (LastScrollTime == 0) {
			LastScrollTime = FreeRTOS::getTimeSinceStart();//HAL_GetTick();
		}
		//if (HAL_GetTick() - LastScrollTime > TimeBetweenScroll) {
		if (FreeRTOS::getTimeSinceStart() - LastScrollTime > TimeBetweenScroll) {
			LastScrollTime = FreeRTOS::getTimeSinceStart();//HAL_GetTick();
			LastScrollPosition++;
			uint32_t l = strlen(text);
			//char b[10];
			//sprintf(&b[0],"%d",l);
			if (LastScrollPosition >= l) {
				LastScrollPosition = 0;
			}
		}
		offSet = LastScrollPosition;
	}
	return text + offSet;
}

uint8_t GUI::drawList(GUIListData* gui_CurList) const {
	if (gui_CurList == 0)
		return 0;

	Display->fillRec(gui_CurList->x, gui_CurList->y, gui_CurList->w,
			gui_CurList->h, RGBColor::BLACK);
	Display->drawRec(gui_CurList->x, gui_CurList->y, gui_CurList->w - 1,
			gui_CurList->h, RGBColor::BLUE);

	uint8_t ry = gui_CurList->y + 2;
	uint8_t rx = gui_CurList->x + 4;
	if (gui_CurList->header != 0) {
		Display->drawString(rx, ry, gui_CurList->header, RGBColor::WHITE,
				RGBColor::BLACK, 1, false);
		ry += Display->getFont()->FontHeight;
	}

	uint8_t maxC = ((gui_CurList->h - 3) / Display->getFont()->FontHeight)
			- (gui_CurList->header != 0);

	uint16_t i;
	if (maxC >= gui_CurList->ItemsCount) {
		for (i = 0; i < gui_CurList->ItemsCount; i++) {
			if (i != gui_CurList->selectedItem) {
				Display->drawString(rx, ry + i * Display->getFont()->FontHeight,
						gui_CurList->items[i].text, RGBColor::WHITE,
						RGBColor::BLACK, 1, false);
			} else {
				Display->drawString(rx, ry + i * Display->getFont()->FontHeight,
						gui_CurList->items[i].getScrollOffset(),
						RGBColor::BLACK, RGBColor::WHITE, 1, false);
			}
		}
	} else {
		if (gui_CurList->ItemsCount - 1 - gui_CurList->selectedItem
				< maxC / 2) {
			for (i = gui_CurList->ItemsCount - maxC;
					i < gui_CurList->ItemsCount; i++) {
				if (i != gui_CurList->selectedItem) {
					Display->drawString(rx,
							ry
									+ (i - gui_CurList->ItemsCount + maxC)
											* Display->getFont()->FontHeight,
							gui_CurList->items[i].text, RGBColor::WHITE,
							RGBColor::BLACK, 1, false);
				} else {
					Display->drawString(rx,
							ry
									+ (i - gui_CurList->ItemsCount + maxC)
											* Display->getFont()->FontHeight,
							gui_CurList->items[i].getScrollOffset(),
							RGBColor::BLACK, RGBColor::WHITE, 1, false);
				}
			}
		} else if (gui_CurList->selectedItem < maxC / 2) {
			for (i = 0; i < maxC; i++) {
				if (i != gui_CurList->selectedItem)
					Display->drawString(rx,
							ry + i * Display->getFont()->FontHeight,
							gui_CurList->items[i].text, RGBColor::WHITE,
							RGBColor::BLACK, 1, false);
				else
					Display->drawString(rx,
							ry + i * Display->getFont()->FontHeight,
							gui_CurList->items[i].getScrollOffset(),
							RGBColor::BLACK, RGBColor::WHITE, 1, false);
			}
		} else {
			for (i = gui_CurList->selectedItem - maxC / 2;
					i < gui_CurList->selectedItem - maxC / 2 + maxC; i++) {
				if (i != gui_CurList->selectedItem) {
					Display->drawString(rx,
							ry
									+ (i - gui_CurList->selectedItem + maxC / 2)
											* Display->getFont()->FontHeight,
							gui_CurList->items[i].text, RGBColor::WHITE,
							RGBColor::BLACK, 1, false);
				} else {
					Display->drawString(rx,
							ry
									+ (i - gui_CurList->selectedItem + maxC / 2)
											* Display->getFont()->FontHeight,
							gui_CurList->items[i].getScrollOffset(),
							RGBColor::BLACK, RGBColor::WHITE, 1, false);
				}
			}
		}
	}
	uint8_t sli_h = gui_CurList->h / gui_CurList->ItemsCount;
	if (sli_h < 14)
		sli_h = 14;
	Display->drawHorizontalLine(gui_CurList->x + 2, ry - 2,
			gui_CurList->x + gui_CurList->w - 2, RGBColor::WHITE);
	return 0;
}

