#ifndef LIB_ESP32_INET_H
#define LIB_ESP32_INET_H

#include "../error_type.h"
#include <esp_netif.h>

namespace libesp {

class ESP32INet {
public:
	static ESP32INet *get();
	static void dumpToLog();
public:
	~ESP32INet();
	esp_netif_t *createWifiInterfaceAP();
	esp_netif_t *createWifiInterfaceSTA();
	ErrorType init() const;
	ErrorType shutdown() const;
private:
	ESP32INet();
	static ESP32INet *mSelf;
	static const char *LOGTAG;
	esp_netif_t *APInterface;
	esp_netif_t *STAInterface;
};

}

#endif
