#ifndef LIBESP_DEVICE_DISPLAY_LAYOUT_H
#define LIBESP_DEVICE_DISPLAY_LAYOUT_H

namespace libesp {

class Widget {
public:
	Widget(uint16_t w, uint16_t h, const BoundingVolume &bv);
	void draw(DisplayDevice *d, uint16_t sx, uint16_t sy);
	bool pick(const Point2Dus &pickPt);
	bool isVisible(Point2Dus &scrollPos);
};

class Layout {
public:
	Layout(uint16_t w, uint16_t h, bool bShowScrollIfNeeded);
	void add(Widget *w);
	void draw(DisplayDevice *d);
private:
	std::vector<Widget *> Widgets;
	Point2Ds ScrollOffSet;
};
 
}
#endif
