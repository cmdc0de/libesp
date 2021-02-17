
#ifndef LIBESP32_BOUNDING_VOLUME_H
#define LIBESP32_BOUNDING_VOLUME_H

#include "point.h"

namespace libesp {

/*
 *  Base class for 2d bounding volumes
 */

class RGBColor;
class DisplayDevice;

class BoundingVolume2D {
public:
	bool containsPoint(const Point2Ds &p) const {
		return onContainsPoint(p);
	}
	bool containsPoint(const Point2Dus &p) const {
		return onContainsPoint(p);
	}
	void updateWorldCoordinates(const Point2Ds &p) {
		return onUpdateWorldCoordinates(p);
	}
	void draw(DisplayDevice *d, const RGBColor &c, bool bFill) const {
		return onDraw(d,c,bFill);
	}
	const Point2Ds &getCenter() const {return onGetCenter();}
	virtual ~BoundingVolume2D() {}
protected:
	virtual bool onContainsPoint(const Point2Ds &p) const =0;
	virtual bool onContainsPoint(const Point2Dus &p) const =0;
	virtual void onUpdateWorldCoordinates(const Point2Ds &p)=0;
	virtual void onDraw(DisplayDevice *d, const RGBColor &c, bool bFill) const=0;
	virtual const Point2Ds &onGetCenter() const=0;
};

}
#endif
