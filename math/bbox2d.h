#ifndef LIBESP_MATH_BBOX2D_H
#define LIBESP_MATH_BBOX2D_H

#include "bounding_volume.h"

namespace libesp {

/*
 * Axis Aligned 2D bounding box
 * @author: cmdc0de
 */
class AABBox2D : public BoundingVolume2D {
public:
	AABBox2D(const Point2Ds &center, uint16_t extent);
	virtual ~AABBox2D();
	uint16_t getExtent() const {return Extent;}
protected:
	virtual bool onContainsPoint(const Point2Dus &p) const;
	virtual bool onContainsPoint(const Point2Ds &p) const;
	virtual void onUpdateWorldCoordinates(const Point2Ds &p);
private:
	Point2Ds Center;
	uint16_t Extent;
};

}

#endif
