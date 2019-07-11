#ifndef LIBESP_MATH_POINT_H
#define LIBESP_MATH_POINT_H

#include <stdint.h>

namespace libesp {
template<typename T>
class Point2D {
public:
	Point2D() : X(T(0)), Y(T(0)) {}
	Point2D(const T &x, const T &y) : X(x), Y(y) {;}
	T getX() { return X; }
	T getY() { return Y; }
private:
	T X;
	T Y;
};

using Point2Dus = Point2D<uint16_t>;
using Point2Ds = Point2D<int16_t>;
}

#endif
