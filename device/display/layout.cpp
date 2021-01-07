#include "layout.h"
#include <device/display/display_device.h>

using namespace libesp;

Widget::Widget(const BoundingVolume2D &bv) : BVolume(&bv) {

}


void Widget::draw(DisplayDevice *d, uint16_t sx, uint16_t sy) const {
	onDraw(d,sx,sy);
}


bool Widget::pick(const Point2Dus &pickPt) const {
	return BVolume->containsPoint(pickPt);
}

Widget::~Widget() {
	BVolume = nullptr;
}

/*
 *
 */

Button::Button(const AABBox2D &bv, const RGBColor &notSelected, const RGBColor &selected) : Widget(bv), NotSelected(notSelected), Selected(selected) {

}

const AABBox2D *Button::getBox() const {
	return (const AABBox2D*)(getBoundingVolume());
}

ErrorType Button::onDraw(DisplayDevice *d, uint16_t sx, uint16_t sy) const {
	ErrorType et;
	d->fillRec((int16_t)sx, (int16_t)sy, (int16_t)sx+getBox()->getExtent(), (int16_t)sy+getBox()->getExtent(), NotSelected);
	return et;
}


Layout::Layout(uint16_t w, uint16_t h, bool bShowScrollIfNeeded) : Width(w), Height(h), ShowScrollIfNeeded(bShowScrollIfNeeded), ScrollOffSet() {

}


void Layout::add(const Widget *w) {
	onAdd(w);
}

void Layout::draw(DisplayDevice *d) {
	onDraw(d);
}

/*
 *
 */

StaticGridLayout::StaticGridLayout(DrawInfo *dIWidgets, uint8_t numWidgets, uint8_t minPixelsBetweenWidgets, uint16_t w, uint16_t h, bool bShowScrollIfNeeded)
	: Layout(w,h,bShowScrollIfNeeded), MinPixelsBetweenWidgets(minPixelsBetweenWidgets), DIWidgets(dIWidgets), NumWidgets(numWidgets) {

}

StaticGridLayout::~StaticGridLayout() {
	DIWidgets = nullptr;
}

void StaticGridLayout::init() {
	DIWidgets[0].setX(MinPixelsBetweenWidgets);
	DIWidgets[0].setY(MinPixelsBetweenWidgets);
}


void StaticGridLayout::onDraw(DisplayDevice *d) {
	for(int i=0;i<NumWidgets;++i) {
		DIWidgets[i].getWidget()->draw(d, DIWidgets[i].getX(), DIWidgets[i].getY());
	}
}

