#ifndef LIBESP_DEVICE_DISPLAY_LAYOUT_H
#define LIBESP_DEVICE_DISPLAY_LAYOUT_H

#include "../../error_type.h"
#include "../../math/bbox2d.h"
#include "color.h"

namespace libesp {

class DisplayDevice;

class Widget {
public:
	Widget(BoundingVolume2D *bv, const uint16_t &widgetID, const char *name);
	void setWorldCoordinates(const Point2Ds &pt);
	const Point2Ds &getWorldCoordinates() const {return StartingPoint;}
	uint16_t getWidgetID() {return WidgetID;}
	void draw(DisplayDevice *d) const;
	bool pick(const Point2Ds &pickPt) const;
	const BoundingVolume2D *getBoundingVolume() const {return BVolume;}
	const char *getName() const {return Name;}
	int16_t getNameLength() const {return NameLen;}
	virtual ~Widget();
protected:
	virtual ErrorType onDraw(DisplayDevice *d) const=0;
private:
	BoundingVolume2D *BVolume;
	uint16_t WidgetID;
	Point2Ds StartingPoint;
	const char *Name;
	int16_t NameLen;
};

class Button : public Widget {
public:
	static const char *LOGTAG;
	Button(const char *name, const uint16_t &wID, AABBox2D *bv, const RGBColor &notSelected, const RGBColor &seleted);
	virtual ~Button() {}
protected:
	virtual ErrorType onDraw(DisplayDevice *d) const override;
	AABBox2D *getBox();
	const AABBox2D *getBox() const;
private:
	RGBColor NotSelected;
	RGBColor Selected;
};

class Layout {
public:
	Layout(uint16_t w, uint16_t h, bool bShowScrollIfNeeded);
	void add(const Widget *w);
	void draw(DisplayDevice *d);
	Widget *pick(const Point2Ds &pickPt);
	virtual ~Layout() {}
protected:
	virtual void onAdd(const Widget *w)=0;
	virtual void onDraw(DisplayDevice *d)=0;
	virtual Widget *onPick(const Point2Ds &pickPt)=0;
private:
	uint16_t Width;
	uint16_t Height;
	bool ShowScrollIfNeeded;
	Point2Ds ScrollOffSet;
};

class StaticGridLayout : public Layout {
public:
	StaticGridLayout(Widget **Widgets, uint8_t numWidgets, uint16_t w, uint16_t h, bool bShowScrollIfNeeded);
	virtual ~StaticGridLayout();
	void init();
protected:
	virtual void onAdd(const Widget *w) {/*do nothing*/}
	virtual void onDraw(DisplayDevice *d);
	virtual Widget *onPick(const Point2Ds &pickPt);
private:
	Widget **Widgets;
	uint8_t NumWidgets;
};

}
#endif
