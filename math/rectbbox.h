#ifndef LIBESP_MATH_RECTBOX_H
#define LIBESP_MATH_RECTBOX_H

#include "bounding_volume.h"

namespace libesp {

class RectBBox2D : public BoundingVolume2D {
public:
	static const char *LOGTAG;
public:
	RectBBox2D(const Point2Ds &center, uint16_t width, uint16_t height);
	virtual ~RectBBox2D();
	Point2Ds getTopLeft() const;
	Point2Ds getBottomRight() const;
	const Point2Ds &getCenter() const {return Center;}
	uint16_t getWidth() const {return Width;}
	uint16_t getHeight() const {return Height;}
protected:
	virtual bool onContainsPoint(const Point2Dus &p) const;
	virtual bool onContainsPoint(const Point2Ds &p) const;
	virtual void onUpdateWorldCoordinates(const Point2Ds &p);
	virtual void onDraw(DisplayDevice *d, const RGBColor &c, bool bFill) const;
	virtual const Point2Ds &onGetCenter() const {return getCenter();}
private:
	Point2Ds Center;
	uint16_t Width;
	uint16_t Height;
};

}

#endif
