#include "layout.h"
#include <device/display/display_device.h>

using namespace libesp;

Widget::Widget(const uint16_t &wid, const char *name) : WidgetID(wid), StartingPoint(), Name(name), NameLen(0), Traits(), Hidden(false) {
	NameLen = static_cast<int16_t>(strlen(name));
}

Widget::Widget(const Widget &r) : WidgetID(r.WidgetID), StartingPoint(r.StartingPoint), Name(r.Name), NameLen(r.NameLen), Traits(), Hidden(r.Hidden) {
	for(int i=0;i<TOTAL_TRAITS;++i) {
		Traits[i] = r.Traits[i];
	}
}


void Widget::draw(DisplayDevice *d) const {
	onDraw(d);
}


void Widget::reset() {
	onReset();
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

Button::Button(const char *name, const uint16_t &wID, BoundingVolume2D *bv, const RGBColor &notSelected, const RGBColor &selected)
	: Widget(wID,name), NotSelected(notSelected), Selected(selected), ButtonSelected(false) {
	std::shared_ptr<Trait> sp(new Pickable2D(bv));
	addTrait(PICKABLE2D,sp);
	addTrait(BVTrait,sp);
}

Button::Button(const Button &r) : Widget(r), NotSelected(r.NotSelected), Selected(r.Selected), ButtonSelected(false) {

}

ErrorType Button::onDraw(DisplayDevice *d) const {
	ErrorType et;
	const BVolumeTrait *bvt = getBVTrait();
	if(bvt) {
		getBVTrait()->draw(d,NotSelected,true);
		/*center text for now*/
		int16_t startY = getBVTrait()->getCenter().getY() - (d->getFont()->FontHeight/2);
		int16_t startX = getBVTrait()->getCenter().getX() - (d->getFont()->FontWidth*getNameLength()/2);
		d->drawString(startX,startY,getName(), RGBColor::WHITE, ButtonSelected?Selected:NotSelected, 1, true);
	} else {
		et = ErrorType(ErrorType::NO_BOUNDING_VOLUME);
	}
	return et;
}

void Button::onReset() {

}

//////////////////////////////////////////////////////////////
const char *CountDownTimer::LOGTAG = "CountDownTimer";

CountDownTimer::CountDownTimer(BoundingVolume2D *bv, const char *name, const uint16_t &wID, uint16_t numSec)
: Widget(wID,name), NumSeconds(numSec*1000), StartTime(0), SecondsLeft(NumSeconds), State(STOPPED), ShowMS(false) {
	std::shared_ptr<Trait> sp(new BVolumeTrait(bv));
	addTrait(BVTrait,sp);
}

void CountDownTimer::startTimer() {
	State = RUNNING;
	StartTime = esp_timer_get_time();
	SecondsLeft = NumSeconds;
}

bool CountDownTimer::isDone() {
	bool bRet = false;
	if(State==TIMER_DONE) {
		bRet = true;
	}
	return bRet;
}

void CountDownTimer::stopTimer() {
	State = TIMER_DONE;
}

void CountDownTimer::update() {
	if(State==RUNNING) {
		//ESP_LOGI(LOGTAG,"%lld %lld %lld %lld",esp_timer_get_time(),StartTime,v,SecondsLeft);
		SecondsLeft = NumSeconds - ((esp_timer_get_time()-StartTime)/1000);
		if(SecondsLeft<=0) {
			stopTimer();
			SecondsLeft=0;
		}
	}
}

ErrorType CountDownTimer::onDraw(DisplayDevice *d) const {
	ErrorType et;

	//convert min.sec.ms
	int32_t min = SecondsLeft/60000;
	int32_t sec = (SecondsLeft/1000)%60;

	char DisplayString[24] = {'\0'};

	if(showMS()) {
		int32_t ms = SecondsLeft%1000;
		sprintf(&DisplayString[0],"%.2d:%.2d.%3d",min,sec,ms);
	} else {
		sprintf(&DisplayString[0],"%.2d:%.2d",min,sec);
	}
	getBVTrait()->draw(d,RGBColor::BLUE,true);
			/*center text for now*/
	int16_t startY = getBVTrait()->getCenter().getY() - (d->getFont()->FontHeight);
	int16_t startX = getBVTrait()->getCenter().getX() - (d->getFont()->FontWidth*15/2); //5=total characters for clock * 3 x size
	d->drawString(startX,startY,&DisplayString[0], RGBColor::BLACK, RGBColor::BLUE, 3, true);
	return et;
}

void CountDownTimer::onReset() {
	State=STOPPED;
}

//////////////////////////////////////////////////////

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

void Layout::reset() {
	return onReset();
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

void StaticGridLayout::onDraw(DisplayDevice *d) {
	for(int i=0;i<NumWidgets;++i) {
		if(!Widgets[i]->isHidden()) {
			Widgets[i]->draw(d);
		}
	}
}

Widget *StaticGridLayout::onPick(const Point2Ds &pickPt) {
	for(int i=0;i<NumWidgets;++i) {
		if(!Widgets[i]->isHidden()) {
			if(Widgets[i]->pick(pickPt)) {
				return Widgets[i];
			}
		}
	}
	return nullptr;
}

void StaticGridLayout::onReset() {
	for(int i=0;i<NumWidgets;++i) {
		Widgets[i]->reset();
	}
}
