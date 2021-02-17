#ifndef LIBESP_DEVICE_DISPLAY_LAYOUT_H
#define LIBESP_DEVICE_DISPLAY_LAYOUT_H

#include "../../error_type.h"
#include "../../math/bbox2d.h"
#include "color.h"
#include <memory>

namespace libesp {

class DisplayDevice;

class Trait {
public:
	class TraitUpdateContext {
	public:
		TraitUpdateContext(const uint32_t &traitUpdates, const Point2Ds *positionUpdate) : TraitUpdates(traitUpdates), PositionUpdate(positionUpdate) {}
		const Point2Ds &getPositionUpdate() const {return *PositionUpdate;}
	private:
		uint32_t TraitUpdates;
		const Point2Ds *PositionUpdate;
	};
	enum TRAIT_UPDATES {
		POSITIONAL_UPDATES = (1<<0)
	};
public:
	Trait() {}
	virtual ~Trait(){}
	ErrorType updateTrait(uint32_t TraitUpdates, const TraitUpdateContext &tuc);
protected:
	virtual ErrorType doUpdateTrait(const TraitUpdateContext &tuc)=0;
};

class BVolumeTrait : public Trait {
public:
	static const uint32_t UPDATES_I_CARE_ABOUT = POSITIONAL_UPDATES;
public:
	BVolumeTrait(BoundingVolume2D *bv) : BVolume(bv) {}
	const BoundingVolume2D *getBoundingVolume() const {return BVolume;}
	BoundingVolume2D *getBoundingVolume() {return BVolume;}
	void draw(DisplayDevice *d, const RGBColor &c, bool bFill) const { BVolume->draw(d, c, bFill);}
	const Point2Ds &getCenter() const {return BVolume->getCenter();}
	virtual ~BVolumeTrait() {}
protected:
	virtual ErrorType doUpdateTrait(const TraitUpdateContext &tuc) {
		ErrorType et;
		BVolume->updateWorldCoordinates(tuc.getPositionUpdate());
		return et;
	}
private:
	BoundingVolume2D *BVolume;
};

class Pickable2D : public BVolumeTrait {
public:
	static const uint32_t UPDATES_I_CARE_ABOUT = POSITIONAL_UPDATES;
public:
	Pickable2D(BoundingVolume2D *bv) : BVolumeTrait(bv) {}
	virtual ~Pickable2D() {}
};

class Widget {
public:
	enum TRAITS {
		PICKABLE2D = 0
		, BVTrait
		, TOTAL_TRAITS
	};
public:
	Widget(const uint16_t &widgetID, const char *name);
	Widget(const Widget &r);
	void setWorldCoordinates(const Point2Ds &pt);
	const Point2Ds &getWorldCoordinates() const {return StartingPoint;}
	uint16_t getWidgetID() {return WidgetID;}
	void draw(DisplayDevice *d) const;
	bool pick(const Point2Ds &pickPt) const;
	const char *getName() const {return Name;}
	int16_t getNameLength() const {return NameLen;}
	void addTrait(TRAITS t, std::shared_ptr<Trait> &trait);
	Pickable2D *getPickable();
	const Pickable2D *getPickable() const;
	BVolumeTrait *getBVTrait();
	const BVolumeTrait *getBVTrait() const;
	virtual ~Widget();
protected:
	virtual ErrorType onDraw(DisplayDevice *d) const=0;
private:
	uint16_t WidgetID;
	Point2Ds StartingPoint;
	const char *Name;
	int16_t NameLen;
	std::shared_ptr<Trait> Traits[TOTAL_TRAITS];
};

class Button : public Widget {
public:
	static const char *LOGTAG;
	Button(const char *name, const uint16_t &wID, AABBox2D *bv, const RGBColor &notSelected, const RGBColor &seleted);
	Button(const Button &r);
	virtual ~Button() {}
protected:
	virtual ErrorType onDraw(DisplayDevice *d) const override;
	AABBox2D *getBox();
	const AABBox2D *getBox() const;
private:
	RGBColor NotSelected;
	RGBColor Selected;
};

class CountDownTimer : public Widget {
public:
	enum STATE {
		STOPPED = 0
		, RUNNING = 1
	};
public:
	static const char *LOGTAG;
	CountDownTimer(BoundingVolume2D *bv, const char *name, const uint16_t &wID, uint16_t numSec);
	void setTime(uint16_t t) {NumSeconds = t;}
	uint16_t getTime() {return NumSeconds;}
	void startTimer();
	bool isDone();
	virtual ~CountDownTimer() {}
protected:
	virtual ErrorType onDraw(DisplayDevice *d) const override;
private:
	int64_t NumSeconds;
	int64_t StartTime;
	STATE State;
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
