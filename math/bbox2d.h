#ifndef LIBESP_MATH_BBOX2D_H
#define LIBESP_MATH_BBOX2D_H

#include "point.h"

namespace libesp {
/*
 * Axis Aligned 2D bounding box
 * @author: cmdc0de
 */
class AABBox2D {
public:
	AABBox2D(const Point2Ds &center, uint16_t extent);
	~AABBox2D();
public:
	bool collide(const Point2D &p);
	bool collide(const BBox2D &b);
private:
	Point2Ds Center;
	uint16_t Extent;
};

}

#endif
