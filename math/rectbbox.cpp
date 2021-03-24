#include "rectbbox.h"
#include <device/display/display_device.h>
#include <cmath>
#include <esp_log.h>

using namespace libesp;

const char *RectBBox2D::LOGTAG = "RectBBox2D";

///////////////////////////////////////////////////////////////
template<typename Z>
static bool testContainPoint(const Point2Ds &tl, const Point2Ds &br, const Point2D<Z> &pt) {
	if( (tl.getX()<=pt.getX()) && br.getX()>=pt.getX()) {
		if(tl.getY()<=pt.getY() && br.getY()>=pt.getY()) {
			return true;
		}
	}
	return false;
}

RectBBox2D::RectBBox2D(const Point2Ds &center, uint16_t width, uint16_t height) : Center(center), Width(width), Height(height) {

}

RectBBox2D::~RectBBox2D() {
}

bool RectBBox2D::onContainsPoint(const Point2Dus &p) const {
	return testContainPoint(getTopLeft(),getBottomRight(),p);
}


bool RectBBox2D::onContainsPoint(const Point2Ds &p) const {
	return testContainPoint(getTopLeft(),getBottomRight(),p);
}

void RectBBox2D::onUpdateWorldCoordinates(const Point2Ds &p) {
//	uint16_t halfE = getExtent()/2;
	ESP_LOGI(LOGTAG,"before update world cords: %d %d", Center.getX(), Center.getY());
	Center+=p;
	ESP_LOGI(LOGTAG,"after update world cords: %d %d", Center.getX(), Center.getY());
}

void RectBBox2D::onDraw(DisplayDevice *d, const RGBColor &c, bool bFill) const {
	Point2Ds pt = getTopLeft();
	Point2Ds botRight = getBottomRight();
	if(bFill) {
		d->fillRec(pt.getX(), pt.getY(), botRight.getX()-pt.getX(), botRight.getY()-pt.getY(), c);
	} else {
		d->drawRec(pt.getX(), pt.getY(), botRight.getX()-pt.getX(), botRight.getY()-pt.getY(), c);
	}
}

Point2Ds RectBBox2D::getTopLeft() const {
	return Point2Ds(Center.getX()-getWidth(),Center.getY()-getHeight());
}

Point2Ds RectBBox2D::getBottomRight() const {
	return Point2Ds(Center.getX()+getWidth(),Center.getY()+getHeight());
}

