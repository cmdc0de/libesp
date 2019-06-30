/*
 * color.h
 *
 *  Created on: Jun 29, 2019
 *      Author: cmdc0de
 */

#ifndef LIBESP_DEVICE_DISPLAY_COLOR_H_
#define LIBESP_DEVICE_DISPLAY_COLOR_H_

#include <stdint.h>

namespace libesp {

struct DCImage {
	unsigned int width;
	unsigned int height;
	unsigned int bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
	const char *pixel_data;
};

/*
 * @Author cmdc0de
 * @date:  2/13/17
 *
 * convenient RGB color class
 */
class RGBColor {
public:
	static const RGBColor BLACK;
	static const RGBColor RED;
	static const RGBColor GREEN;
	static const RGBColor BLUE;
	static const RGBColor WHITE;
public:
	RGBColor(uint8_t r, uint8_t g, uint8_t b) :
			R(r), G(g), B(b) {
	}
	RGBColor(const RGBColor &r) {
		(*this) = r;
	}
	uint16_t getR() const {
		return R;
	}
	uint16_t getG() const {
		return G;
	}
	uint16_t getB() const {
		return B;
	}
	RGBColor &operator=(const RGBColor &r) {
		R = r.getR();
		G = r.getG();
		B = r.getB();
		return *this;
	}
	bool operator==(const RGBColor &r) const;
	bool operator!=(const RGBColor &r) const;
private:
	uint8_t R, G, B;
};

}

#endif /* COMPONENTS_LIBESP_DEVICE_DISPLAY_COLOR_H_ */
