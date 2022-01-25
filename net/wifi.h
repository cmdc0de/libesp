/*
 * WiFi.h
 */
#ifndef _LIBESP_WIFI_H_
#define _LIBESP_WIFI_H_

#include <etl/string.h>
#include <etl/vector.h>
#include "../error_type.h"
#include "../freertos.h"
#include "wifieventhandler.h"

namespace libesp {

class WiFiAPRecord {
public:
  typedef etl::string<64>  SSID_TYPE;
  typedef etl::string<256> TO_STRING_TYPE;
public:
    friend class WiFi;
    /**
     * @brief Get the auth mode.
     * @return The auth mode.
     */
    wifi_auth_mode_t getAuthMode() {
        return m_authMode;
    }
    /**
     * @brief Get the RSSI.
     * @return the RSSI.
     */
    int8_t getRSSI() {
        return m_rssi;
    }
    /**
     * @brief Get the SSID.
     * @return the SSID.
     */
    SSID_TYPE getSSID() {
        return m_ssid;
    }
    TO_STRING_TYPE toString();

    void setPrimaryChannel(uint8_t c) {mPrimaryChannel=c;}
    uint8_t getPrimary() {return mPrimaryChannel;}
    void setFlags(uint32_t f) { mFlags = f;}
    bool isWirelessB() { return mFlags & (1<<0);}
    bool isWirelessG() { return mFlags & (1<<1);}
    bool isWirelessN() { return mFlags & (1<<2);}
    bool isWirelessLR() { return mFlags & (1<<3);}
    bool isWPS() { return mFlags & (1<<4);}
    bool isFTMReponder() { return mFlags & (1<<5);}
    bool isFTMInitiator() { return mFlags & (1<<6);}
private:
    uint8_t          m_bssid[6];
    int8_t           m_rssi;
    SSID_TYPE       m_ssid;
    wifi_auth_mode_t m_authMode;
    uint8_t         mPrimaryChannel;
    uint32_t        mFlags;
};


/**
 * @brief %WiFi driver.
 *
 * Encapsulate control of %WiFi functions.
 *
 * Here is an example fragment that illustrates connecting to an access point.
 * @code{.cpp}
 * #include <WiFi.h>
 * #include <WiFiEventHandler.h>
 *
 * class TargetWiFiEventHandler: public WiFiEventHandler {
 *    esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
 *       ESP_LOGD(tag, "MyWiFiEventHandler(Class): staGotIp");
 *       // Do something ...
 *       return ESP_OK;
 *    }
 * };
 *
 * WiFi wifi;
 *
 * TargetWiFiEventHandler *eventHandler = new TargetWiFiEventHandler();
 * wifi.setWifiEventHandler(eventHandler);
 * wifi.connectAP("myssid", "mypassword");
 * @endcode
 */
class WiFi {
public:
  static const char *LOGTAG;
public:
	WiFi();
	ErrorType init(const wifi_mode_t &wmode);
  ErrorType scan(etl::vector<WiFiAPRecord,16> &results, bool showHidden);
  ErrorType scan(etl::vector<WiFiAPRecord,16> &results,  const wifi_scan_config_t &conf);
  ErrorType connect(const etl::string<32> &ssid, const etl::string<64> &pass, wifi_auth_mode_t mode);
	~WiFi();
	bool startAP(const std::string& ssid, const std::string& passwd, wifi_auth_mode_t auth = WIFI_AUTH_OPEN);
	bool startAP(const std::string& ssid, const std::string& passwd, wifi_auth_mode_t auth, uint8_t channel, bool ssid_hidden, uint8_t max_connection);
	void setWifiEventHandler(WiFiEventHandler *wifiEventHandler);
	WiFiEventHandler *getWifiEventHandler();
	bool shutdown();
	bool stopWiFi();
  ErrorType setWiFiStorage(wifi_storage_t storage);
protected:
  ErrorType initSTA();
private:
	static void eventHandler(void* ctx, esp_event_base_t event_base, int32_t event_id, void *event_data);
private:
	WiFiEventHandler *MyWiFiEventHandler;
  wifi_mode_t WiFiMode;
};

}

#endif /* MAIN_WIFI_H_ */
