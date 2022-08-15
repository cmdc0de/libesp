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
   esp_netif_t *getWifiAPInterface() {return APInterface;}
   esp_netif_t *getWifiSTAInterface() {return STAInterface;}
	ErrorType init() const;
	ErrorType shutdown() const;
   ErrorType getIPv4(esp_netif_t *i, esp_ip4_addr_t &ip);
   ErrorType getIPv4(esp_netif_t *i, char *buf, uint32_t size);
   ErrorType getNetMask(esp_netif_t *i, esp_ip4_addr_t &ip);
   ErrorType getNetMask(esp_netif_t *i, char *buf, uint32_t size);
   ErrorType getGateway(esp_netif_t *i, esp_ip4_addr_t &ip);
   ErrorType getGateway(esp_netif_t *i, char *buf, uint32_t size);
   ErrorType getMacAddress(esp_netif_t *i, uint8_t buf[16]);

private:
	ESP32INet();
	static ESP32INet *mSelf;
	static const char *LOGTAG;
	esp_netif_t *APInterface;
	esp_netif_t *STAInterface;
};

}

#endif
