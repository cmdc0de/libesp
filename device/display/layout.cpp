#include "layout.h"
#include <device/display/display_device.h>

using namespace libesp;

Widget::Widget(const uint16_t &wid, const char *name) : WidgetID(wid), StartingPoint(), Name(name), NameLen(0), Traits() {
	NameLen = static_cast<int16_t>(strlen(name));
}


void Widget::draw(DisplayDevice *d) const {
	onDraw(d);
}


bool Widget::pick(const Point2Ds &pickPt) const {
	const Pickable2D* p = getPickable();
	if(nullptr!=p) {
		return p->getBoundingVolume()->containsPoint(pickPt);
	}
	return false;
}

Widget::~Widget() {
}

void Widget::setWorldCoordinates(const Point2Ds &pt) {
	StartingPoint = pt;
	Pickable2D* p = getPickable();
	if(nullptr!=p) {
		return p->getBoundingVolume()->updateWorldCoordinates(pt);
	}
}

void Widget::addTrait(TRAITS t, std::shared_ptr<Trait> &trait) {
	Traits[t] = trait;
}

Pickable2D * Widget::getPickable() {
	if(nullptr!=Traits[PICKABLE2D].get()) {
		return ((Pickable2D *)Traits[PICKABLE2D].get());
	}
	return nullptr;
}

const Pickable2D * Widget::getPickable() const {
	if(nullptr!=Traits[PICKABLE2D].get()) {
		return ((Pickable2D *)Traits[PICKABLE2D].get());
	}
	return nullptr;
}

/*
 *
 */

const char *Button::LOGTAG = "Button";

Button::Button(const char *name, const uint16_t &wID, AABBox2D *bv, const RGBColor &notSelected, const RGBColor &selected)
	: Widget(wID,name), NotSelected(notSelected), Selected(selected) {
	std::shared_ptr<Trait> sp(new Pickable2D(bv));
	addTrait(PICKABLE2D,sp);

}

AABBox2D *Button::getBox() {
	Pickable2D* p = getPickable();
	if(nullptr!=p) {
		return ((AABBox2D*)p->getBoundingVolume());
	}
	return nullptr;
}

const AABBox2D *Button::getBox() const {
	const Pickable2D* p = getPickable();
	if(nullptr!=p) {
		return ((AABBox2D*)p->getBoundingVolume());
	}
	return nullptr;
}

ErrorType Button::onDraw(DisplayDevice *d) const {
	ErrorType et;
	Point2Ds pt = getBox()->getTopLeft();
	Point2Ds botRight = getBox()->getBottomRight();
	//ESP_LOGI(LOGTAG, "%s, start: %d:%d, w/h: %d:%d", getName(), pt.getX(), pt.getY(), botRight.getX()-pt.getX(), botRight.getY()-pt.getY());
	d->fillRec(pt.getX(), pt.getY(), botRight.getX()-pt.getX(), botRight.getY()-pt.getY(), NotSelected);
	//d->drawRec(pt.getX(), pt.getY(), botRight.getX()-pt.getX(), botRight.getY()-pt.getY(), NotSelected);
	//ESP_LOGI(LOGTAG, "%s: %d:%d %d:%d\n", getName(),pt.getX(),pt.getY(), botRight.getX(), botRight.getY());
	/*center text for now*/
	int16_t startY = getBox()->getCenter().getY() - (d->getFont()->FontHeight/2);
	int16_t startX = getBox()->getCenter().getX() - (d->getFont()->FontWidth*getNameLength()/2);
	d->drawString(startX,startY,getName(), RGBColor::WHITE, NotSelected, 1, true);
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

StaticGridLayout::StaticGridLayout(Widget **dIWidgets, uint8_t numWidgets, uint16_t w, uint16_t h, bool bShowScrollIfNeeded)
	: Layout(w,h,bShowScrollIfNeeded), Widgets(dIWidgets), NumWidgets(numWidgets) {

}

StaticGridLayout::~StaticGridLayout() {
	Widgets = nullptr;
}

void StaticGridLayout::init() {
	/*
	 *Point2Ds pt(MinPixelsBetweenWidgets,MinPixelsBetweenWidgets);
	for(int i=0;i<NumWidgets;++i) {
		Widgets[i]->setWorldCoordinates(pt);
	}
	 */
}


void StaticGridLayout::onDraw(DisplayDevice *d) {
	for(int i=0;i<NumWidgets;++i) {
		Widgets[i]->draw(d);
	}
}

Widget *StaticGridLayout::onPick(const Point2Ds &pickPt) {
	for(int i=0;i<NumWidgets;++i) {
		if(Widgets[i]->pick(pickPt)) {
			return Widgets[i];
		}
	}
	return nullptr;
}

