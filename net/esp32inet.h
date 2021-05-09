#ifndef LIB_ESP32_INET_H
#define LIB_ESP32_INET_H

#include "../error_type.h"

namespace libesp {

class ESP32INet {
public:
	static const ESP32INet *get();
	static void dumpToLog();
public:
	ESP32INet();
	~ESP32INet();
	ErrorType init() const;
	ErrorType shutdown() const;
private:
	static ESP32INet *mSelf;
	static const char *LOGTAG;
};

}

#endif
