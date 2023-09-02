#pragma once

#include "../../error_type.h"
#include "../../freertos.h"

namespace libesp {

   class DisplayGUIListItemData {
   public:
	   DisplayGUIListItemData(uint8_t id1, const char *msg, bool scroll, uint16_t timeBetwenScrolls) :
			id(id1), text(msg), Scrollable(scroll), TimeBetweenScroll(timeBetwenScrolls), LastScrollTime(0)
         , LastScrollPosition(0) { }
	   DisplayGUIListItemData(uint8_t id1, const char *msg) : id(id1), text(msg), Scrollable(0)
         , TimeBetweenScroll(1000), LastScrollTime(0), LastScrollPosition( 0) { }
	   DisplayGUIListItemData() : id(0), text(0), Scrollable(0), TimeBetweenScroll(1000), LastScrollTime(0)
                       , LastScrollPosition(0) { }
	   void set(uint8_t n, const char *msg) { id = n; text = msg; }
	   const char *getScrollOffset();
	   void setShouldScroll();
	   bool shouldScroll() ;
	   void resetScrollable() { Scrollable = 1; LastScrollTime = 0; LastScrollPosition = 0; }
	   uint16_t id; /*!< Item's id */
	   const char* text; /*!< Item's text*/
	   uint16_t Scrollable :1;
	   uint16_t TimeBetweenScroll :12;
	   uint32_t LastScrollTime;
	   uint8_t LastScrollPosition;
   };
   // 
   class DisplayGUIListData {
   public:
	   DisplayGUIListData(const char *h, DisplayGUIListItemData *is, uint8_t x1, uint8_t y1, uint8_t width
            , uint8_t height, uint8_t si, uint8_t ic) : header(h), items(is), ItemsCount(ic), x(x1), y(y1)
                                                        , w(width), h(height), selectedItem(si) { }
	   const char* header; /*!< Header*/
	   DisplayGUIListItemData *items; /*!< Item's array*/
	   uint16_t ItemsCount; /*!< Item's array*/
	   uint8_t x, y, w, h;
	   uint16_t selectedItem;
	   uint16_t getSelectedItemID() {return items[selectedItem].id;}
	   DisplayGUIListItemData &getSelectedItem() {return items[selectedItem];}
      void moveUp();
      void moveDown();
      void selectTop();
   };
}

