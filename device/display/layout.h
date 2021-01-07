#ifndef LIBESP_DEVICE_DISPLAY_LAYOUT_H
#define LIBESP_DEVICE_DISPLAY_LAYOUT_H

#include "../../error_type.h"
#include "../../math/bbox2d.h"
#include "color.h"

namespace libesp {

class DisplayDevice;

class Widget {
public:
	Widget(const BoundingVolume2D &bv);
	void draw(DisplayDevice *d, uint16_t sx, uint16_t sy) const;
	bool pick(const Point2Dus &pickPt) const;
	virtual ~Widget();
protected:
	virtual ErrorType onDraw(DisplayDevice *d, uint16_t sx, uint16_t sy) const=0;
	const BoundingVolume2D *getBoundingVolume() const {return BVolume;}
private:
	const BoundingVolume2D *BVolume;
};

class Button : public Widget {
public:
	Button(const AABBox2D &bv, const RGBColor &notSelected, const RGBColor &seleted);
	virtual ~Button() {}
protected:
	virtual ErrorType onDraw(DisplayDevice *d, uint16_t sx, uint16_t sy) const override;
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
	virtual ~Layout() {}
protected:
	virtual void onAdd(const Widget *w)=0;
	virtual void onDraw(DisplayDevice *d)=0;
private:
	uint16_t Width;
	uint16_t Height;
	bool ShowScrollIfNeeded;
	Point2Ds ScrollOffSet;
};

class StaticGridLayout : public Layout {
public:
	class DrawInfo {
	public:
		DrawInfo(const Widget *w): StartWidgetX(0), StartWidgetY(0), WidgetToDraw(w) {}
		void setX(uint16_t x) {StartWidgetX=x;}
		void setY(uint16_t y) {StartWidgetY=y;}
		uint16_t getX() const {return StartWidgetX;}
		uint16_t getY() const {return StartWidgetY;}
		const Widget *getWidget() const {return WidgetToDraw;}
	protected:
		uint16_t StartWidgetX;
		uint16_t StartWidgetY;
		const Widget *WidgetToDraw;
		DrawInfo() : StartWidgetX(0), StartWidgetY(0), WidgetToDraw(0) {}
		DrawInfo(uint16_t sx, uint16_t sy, const Widget *w): StartWidgetX(sx), StartWidgetY(sy), WidgetToDraw(w) {}
	};
public:
	StaticGridLayout(DrawInfo *DIWidgets, uint8_t numWidgets, uint8_t minPixelsBetweenWidgets, uint16_t w, uint16_t h, bool bShowScrollIfNeeded);
	virtual ~StaticGridLayout();
	void init();
protected:
	virtual void onAdd(const Widget *w) {/*do nothing*/}
	virtual void onDraw(DisplayDevice *d);
private:
	uint8_t MinPixelsBetweenWidgets;
	DrawInfo *DIWidgets;
	uint8_t NumWidgets;
};

}
#endif
