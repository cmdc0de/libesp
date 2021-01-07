#include "bbox2d.h"
#include <cmath>

using namespace libesp;


AABBox2D::AABBox2D(const Point2Ds &center, uint16_t extent) : Center(center), Extent(extent) {

}

AABBox2D::~AABBox2D() {
}

template<typename T, typename Z> bool testContainPoint(const Point2D<T> &boxCenter, uint32_t extentSquared, const Point2D<Z> &pt) {
	uint32_t distance = std::pow(pt.getX()-boxCenter.getX(),2) + std::pow(pt.getY()-boxCenter.getY(),2);
	return distance<=extentSquared;
}

bool AABBox2D::onContainsPoint(const Point2Dus &p) const {
	return testContainPoint(Center,pow(Extent,2),p);
}


bool AABBox2D::onContainsPoint(const Point2Ds &p) const {
	return testContainPoint(Center,pow(Extent,2),p);
}

void AABBox2D::onUpdateWorldCoordinates(const Point2Ds &p) {
	uint16_t halfE = getExtent()/2;
	Center+=Point2Ds(p.getX()+halfE,p.getY()+halfE);
}
