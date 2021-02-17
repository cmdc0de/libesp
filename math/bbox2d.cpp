#include "bbox2d.h"
#include <device/display/display_device.h>
#include <cmath>
#include <esp_log.h>

using namespace libesp;

static const char *LOGTAG = "AABBOX2D";

AABBox2D::AABBox2D(const Point2Ds &center, uint16_t extent) : Center(center), Extent(extent) {

}

AABBox2D::~AABBox2D() {
}

template<typename T, typename Z> bool testContainPoint(const Point2D<T> &boxCenter, uint32_t extentSquared, const Point2D<Z> &pt) {
	uint32_t distance = std::pow(pt.getX()-boxCenter.getX(),2) + std::pow(pt.getY()-boxCenter.getY(),2);
	ESP_LOGI(LOGTAG, "CX: %d CY: %d PX: %d PY: %d Extent^2: %d", boxCenter.getX(), boxCenter.getY(), pt.getX(), pt.getY(), extentSquared);
	return distance<=extentSquared;
}

bool AABBox2D::onContainsPoint(const Point2Dus &p) const {
	return testContainPoint(Center,pow(Extent,2),p);
}


bool AABBox2D::onContainsPoint(const Point2Ds &p) const {
	return testContainPoint(Center,pow(Extent,2),p);
}

void AABBox2D::onUpdateWorldCoordinates(const Point2Ds &p) {
//	uint16_t halfE = getExtent()/2;
	ESP_LOGI(LOGTAG,"before update world cords: %d %d", Center.getX(), Center.getY());
	Center+=p;
	ESP_LOGI(LOGTAG,"after update world cords: %d %d", Center.getX(), Center.getY());
}

void AABBox2D::onDraw(DisplayDevice *d, const RGBColor &c, bool bFill) const {
	Point2Ds pt = getTopLeft();
	Point2Ds botRight = getBottomRight();
	if(bFill) {
		d->fillRec(pt.getX(), pt.getY(), botRight.getX()-pt.getX(), botRight.getY()-pt.getY(), c);
	} else {
		d->drawRec(pt.getX(), pt.getY(), botRight.getX()-pt.getX(), botRight.getY()-pt.getY(), c);
	}
}

Point2Ds AABBox2D::getTopLeft() const {
	return Point2Ds(Center.getX()-getExtent(),Center.getY()-getExtent());
}

Point2Ds AABBox2D::getBottomRight() const {
	return Point2Ds(Center.getX()+getExtent(),Center.getY()+getExtent());
}
