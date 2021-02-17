#ifndef LIBESP_MATH_BBOX2D_H
#define LIBESP_MATH_BBOX2D_H

#include "bounding_volume.h"

namespace libesp {

/*
 * Axis Aligned 2D bounding box
 * @author: cmdc0de
 *
 * Center Point.X + Extent is max X
 * Center Point.Y + Extent is max y
 * Center Point.X - Extent is min x
 * Center Point.Y - Extent is min y
 *
 */
class AABBox2D : public BoundingVolume2D {
public:
	AABBox2D(const Point2Ds &center, uint16_t extent);
	virtual ~AABBox2D();
	uint16_t getExtent() const {return Extent;}
	Point2Ds getTopLeft() const;
	Point2Ds getBottomRight() const;
	const Point2Ds &getCenter() const {return Center;}
protected:
	virtual bool onContainsPoint(const Point2Dus &p) const;
	virtual bool onContainsPoint(const Point2Ds &p) const;
	virtual void onUpdateWorldCoordinates(const Point2Ds &p);
	virtual void onDraw(DisplayDevice *d, const RGBColor &c, bool bFill) const;
	virtual const Point2Ds &onGetCenter() const {return getCenter();}
private:
	Point2Ds Center;
	uint16_t Extent;
};

}

#endif
