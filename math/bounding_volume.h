
#ifndef LIBESP32_BOUNDING_VOLUME_H
#define LIBESP32_BOUNDING_VOLUME_H

#include "point.h"

namespace libesp {

/*
 *  Base class for 2d bounding volumes
 */

class BoundingVolume2D {
public:
	bool containsPoint(const Point2Ds &p) const {
		return onContainsPoint(p);
	}
	bool containsPoint(const Point2Dus &p) const {
		return onContainsPoint(p);
	}
	virtual ~BoundingVolume2D() {}
protected:
	virtual bool onContainsPoint(const Point2Ds &p) const =0;
	virtual bool onContainsPoint(const Point2Dus &p) const =0;
};

}
#endif
