#include "layout.h"
#include <device/display/display_device.h>

using namespace libesp;

Widget::Widget(BoundingVolume2D *bv, const uint16_t &wid) : BVolume(bv), WidgetID(wid), StartingPoint() {

}


void Widget::draw(DisplayDevice *d) const {
	onDraw(d);
}


bool Widget::pick(const Point2Dus &pickPt) const {
	return BVolume->containsPoint(pickPt);
}

Widget::~Widget() {
	BVolume = nullptr;
}

void Widget::setWorldCoordinates(const Point2Ds &pt) {
	StartingPoint = pt;
	BVolume->updateWorldCoordinates(pt);
}

/*
 *
 */

Button::Button(const char *name, const uint16_t &wID, AABBox2D *bv, const RGBColor &notSelected, const RGBColor &selected)
	: Widget(bv,wID), Name(name), NotSelected(notSelected), Selected(selected) {

}

AABBox2D *Button::getBox() {
	return (AABBox2D*)(getBoundingVolume());
}

ErrorType Button::onDraw(DisplayDevice *d) const {
	ErrorType et;
	Point2Ds pt;
	getBox()->topLeft(pt);
	d->fillRec(pt.getX(), pt.getY(), pt.getX()+getBox()->getExtent(), pt.getY()+getBox()->getExtent(), NotSelected);
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

Widget *Layout::pick(const Point2Ds &pickPt) {
	return onPick(pickPt);
}

/*
 *
 */

StaticGridLayout::StaticGridLayout(Widget **dIWidgets, uint8_t numWidgets, uint8_t minPixelsBetweenWidgets, uint16_t w, uint16_t h, bool bShowScrollIfNeeded)
	: Layout(w,h,bShowScrollIfNeeded), MinPixelsBetweenWidgets(minPixelsBetweenWidgets), Widgets(dIWidgets), NumWidgets(numWidgets) {

}

StaticGridLayout::~StaticGridLayout() {
	Widgets = nullptr;
}

void StaticGridLayout::init() {
	Point2Ds pt(MinPixelsBetweenWidgets,MinPixelsBetweenWidgets);
	for(int i=0;i<NumWidgets;++i) {
		Widgets[i]->setWorldCoordinates(pt);
	}
}


void StaticGridLayout::onDraw(DisplayDevice *d) {
	for(int i=0;i<NumWidgets;++i) {
		Widgets[i]->draw(d);
	}
}

Widget *StaticGridLayout::onPick(const Point2Ds &pickPt) {
	Widget *whit = nullptr;
	for(int i=0;i<NumWidgets;++i) {
		if(Widgets[i]->pick(pickPtr)) {
			return Widgets[i];
		}
	}
	return nullptr;
}

