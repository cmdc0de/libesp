/*
 * WiFi.h
 *
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

#include <string>
#include <vector>
#include <esp_err.h>
#include "../freertos.h"
#include "wifieventhandler.h"

class WiFiAPRecord {
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
    std::string getSSID() {
        return m_ssid;
    }

    std::string toString();

private:
    uint8_t          m_bssid[6];
    int8_t           m_rssi;
    std::string      m_ssid;
    wifi_auth_mode_t m_authMode;
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
    WiFi();
    ~WiFi();
    std::vector<WiFiAPRecord> scan();
    bool                      startAP(const std::string& ssid, const std::string& passwd, wifi_auth_mode_t auth = WIFI_AUTH_OPEN);
    bool                      startAP(const std::string& ssid, const std::string& passwd, wifi_auth_mode_t auth, uint8_t channel, bool ssid_hidden, uint8_t max_connection);
    void                      setWifiEventHandler(WiFiEventHandler *wifiEventHandler);
    WiFiEventHandler *        getWifiEventHandler();
	 bool 							scan(bool bShowHidden);
	 bool shutdown();
	 bool stopWiFi();
private:
	static esp_err_t		eventHandler(void* ctx, system_event_t* event);
	bool 						init();
};

#endif /* MAIN_WIFI_H_ */
