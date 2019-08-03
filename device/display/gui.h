#ifndef GUI_H
#define GUI_H

#include <stdio.h>
#include "display_device.h"
#include "fonts.h"

namespace libesp {

//running line
#define GUI_TickerSpeed 500
#define GUI_TickerEndDelay 3

class GUITickerData {
public:
	GUITickerData(const char * txt, uint8_t X, uint8_t Y, uint8_t W,
			uint8_t H);
	const char *text;
	uint8_t x, y, w, h, BorderSize, FontScalar;
	RGBColor bg, TextColor;
	uint32_t startTick;
};

class GUIListItemData {
public:
	GUIListItemData(uint8_t id1, const char *msg, bool scroll,
			uint16_t timeBetwenScrolls) :
			id(id1), text(msg), Scrollable(scroll), TimeBetweenScroll(
					timeBetwenScrolls), LastScrollTime(0), LastScrollPosition(0) {

	}
	GUIListItemData(uint8_t id, const char *msg) :
			Scrollable(0), TimeBetweenScroll(1000), LastScrollTime(0), LastScrollPosition(
					0) {
		this->id = id;
		text = msg;
	}
	GUIListItemData() :
			id(0), text(0), Scrollable(0), TimeBetweenScroll(1000), LastScrollTime(
					0), LastScrollPosition(0) {
	}
	void set(uint8_t n, const char *msg) {
		id = n;
		text = msg;
	}
	uint16_t id; /*!< Item's id */
	const char* text; /*!< Item's text*/
	uint16_t Scrollable :1;
	uint16_t TimeBetweenScroll :12;
	uint32_t LastScrollTime;
	uint8_t LastScrollPosition;
	const char *getScrollOffset();
	void setShouldScroll();
	bool shouldScroll();
	void resetScrollable() {
		Scrollable = 1;
		LastScrollTime = 0;
		LastScrollPosition = 0;
	}
};

class GUIListData {
public:
	GUIListData(const char *h, GUIListItemData *is, uint8_t x, uint8_t y,
			uint8_t w, uint8_t height, uint8_t si, uint8_t ic) {
		header = h;
		items = is;
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = height;
		selectedItem = si;
		ItemsCount = ic;
	}
	const char* header; /*!< Header*/
	GUIListItemData *items; /*!< Item's array*/
	uint16_t ItemsCount; /*!< Item's array*/
	uint8_t x, y, w, h;
	uint16_t selectedItem;
	uint16_t getSelectedItemID() {return items[selectedItem].id;}
	GUIListItemData &getSelectedItem() {return items[selectedItem];}
};

class GUI {
public:
	GUI(DisplayDevice *display);
	bool init();
	void drawTicker(GUITickerData *dt);
	uint8_t drawList(GUIListData* list) const;

private:
	DisplayDevice *Display;
};

}
#endif
