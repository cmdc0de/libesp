#include "layout.h"
#include <device/display/display_device.h>

using namespace libesp;

Widget::Widget(const uint16_t &wid, const char *name) : WidgetID(wid), StartingPoint(), Name(name), NameLen(0), Traits() {
	NameLen = static_cast<int16_t>(strlen(name));
}

Widget::Widget(const Widget &r) : WidgetID(r.WidgetID), StartingPoint(r.StartingPoint), Name(r.Name), NameLen(r.NameLen), Traits() {
	for(int i=0;i<TOTAL_TRAITS;++i) {
		Traits[i] = r.Traits[i];
	}
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

BVolumeTrait *Widget::getBVTrait() {
	if(nullptr!=Traits[BVTrait].get()) {
		return ((Pickable2D *)Traits[BVTrait].get());
	}
	return nullptr;
}

const BVolumeTrait *Widget::getBVTrait() const {
	if(nullptr!=Traits[BVTrait].get()) {
		return ((Pickable2D *)Traits[BVTrait].get());
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

Button::Button(const Button &r) : Widget(r), NotSelected(r.NotSelected), Selected(r.Selected) {

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
	const BVolumeTrait *bvt = getPickable();
	if(bvt) {
		bvt->draw(d,NotSelected,true);
		/*center text for now*/
		int16_t startY = getBox()->getCenter().getY() - (d->getFont()->FontHeight/2);
		int16_t startX = getBox()->getCenter().getX() - (d->getFont()->FontWidth*getNameLength()/2);
		d->drawString(startX,startY,getName(), RGBColor::WHITE, NotSelected, 1, true);
	} else {
		et = ErrorType(ErrorType::NO_BOUNDING_VOLUME);
	}
	return et;
}

//////////////////////////////////////////////////////////////
const char *CountDownTimer::LOGTAG = "CountDownTimer";

CountDownTimer::CountDownTimer(BoundingVolume2D *bv, const char *name, const uint16_t &wID, uint16_t numSec)
: Widget(wID,name), NumSeconds(numSec*1000), StartTime(0), State(STOPPED) {
	std::shared_ptr<Trait> sp(new BVolumeTrait(bv));
	addTrait(BVTrait,sp);
}

void CountDownTimer::startTimer() {
	State = RUNNING;
	StartTime = esp_timer_get_time();
}

bool CountDownTimer::isDone() {
	bool bRet = false;
	if(State==RUNNING) {
		int64_t v = esp_timer_get_time()-StartTime;
		if(v>NumSeconds) bRet = true;
	}
	return bRet;
}

ErrorType CountDownTimer::onDraw(DisplayDevice *d) const {
	ErrorType et;
	int64_t SecondsLeft = NumSeconds;
	if(State==RUNNING) {
		int64_t v = (esp_timer_get_time()-StartTime);
		SecondsLeft -= v;
	}
	//convert min.sec.ms
	int32_t min = SecondsLeft/60000;
	int32_t sec = (SecondsLeft/1000)%60;
	//int32_t ms = SecondsLeft%1000;
	char DisplayString[24] = {'\0'};
	//sprintf(&DisplayString[0],"%.2d:%.2d.%.3d",min,sec,ms);
	sprintf(&DisplayString[0],"%.2d:%.2d",min,sec);
	getBVTrait()->draw(d,RGBColor::BLUE,true);
			/*center text for now*/
	int16_t startY = getBVTrait()->getCenter().getY() - (d->getFont()->FontHeight);
	int16_t startX = getBVTrait()->getCenter().getX() - (d->getFont()->FontWidth*15/2); //5=total characters for clock * 3 x size
	d->drawString(startX,startY,&DisplayString[0], RGBColor::BLACK, RGBColor::BLUE, 3, true);
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

