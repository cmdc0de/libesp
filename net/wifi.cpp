/*
 * WiFi.cpp
 *
 */

#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "sdkconfig.h"
#include <esp_wifi_types.h>
#include "wifi.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include "../GeneralUtils.h"
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <string.h>


using namespace libesp;
static const char* LOG_TAG = "WiFi";

/**
 * @brief Creates and uses a default event handler
 */
WiFi::WiFi() : MyWiFiEventHandler(nullptr) {
} // WiFi


/**
 * @brief Deletes the event handler that was used by the class
 */
WiFi::~WiFi() {
} // ~WiFi


/**
 * STATIC
  * @brief Primary event handler interface.
  * typedefvoid (*esp_event_handler_t)(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data
 */
void WiFi::eventHandler(void* ctx, esp_event_base_t event_base, int32_t event_id, void *event_data) {

	WiFi *pWiFi = (WiFi *)ctx;   // retrieve the WiFi object from the passed in context.
	ESP_LOGI(LOG_TAG, ">> setWifiEventHandler: 0x%d", (uint32_t)pWiFi->MyWiFiEventHandler);

	if (pWiFi->MyWiFiEventHandler != nullptr) {
		esp_err_t rc = pWiFi->MyWiFiEventHandler->eventHandler(event_base,event_id,event_data);
	}
} 


/**
 * @brief Get the WiFi Mode.
 * @return The WiFi Mode.
 */
/*
std::string WiFi::getMode() {
	wifi_mode_t mode;
	esp_wifi_get_mode(&mode);
	switch(mode) {
		case WIFI_MODE_NULL:
			return "WIFI_MODE_NULL";
		case WIFI_MODE_STA:
			return "WIFI_MODE_STA";
		case WIFI_MODE_AP:
			return "WIFI_MODE_AP";
		case WIFI_MODE_APSTA:
			return "WIFI_MODE_APSTA";
		default:
			return "unknown";
	}
} // getMode
*/

/**
* @brief Initialize WiFi.
*/

bool WiFi::init() {
	bool bRetVal = false;
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_err_t errRC = esp_wifi_init(&cfg);
   if(ESP_OK==errRC) {
		errRC = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandler, this);
		if(ESP_OK==errRC) {
			errRC = ::esp_wifi_set_storage(WIFI_STORAGE_RAM);
			if (errRC == ESP_OK) {
				bRetVal = true;
			} else {
				ESP_LOGE(LOG_TAG, "esp_wifi_set_storage: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
			}
		} else {
			ESP_LOGE(LOG_TAG, "wifi init: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
		}
	} else {
		ESP_LOGE(LOG_TAG, "wifi init: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
	}
	return bRetVal;
} // init


/**
 * @brief Perform a WiFi scan looking for access points.
 *
 * An access point scan is performed and a vector of WiFi access point records
 * is built and returned with one record per found scan instance.  The scan is
 * performed in a blocking fashion and will not return until the set of scanned
 * access points has been built.
 *
 * @return A vector of WiFiAPRecord instances.
 */
std::vector<WiFiAPRecord> WiFi::scan() {
	std::vector<WiFiAPRecord> apRecords;
	return apRecords;
/*
	ESP_LOGD(LOG_TAG, ">> scan");

	init();

	esp_err_t errRc = ::esp_wifi_set_mode(WIFI_MODE_STA);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_wifi_set_mode: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		abort();
	}

	errRc = ::esp_wifi_start();
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_wifi_start: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		abort();
	}

	wifi_scan_config_t conf;
	memset(&conf, 0, sizeof(conf));
	conf.show_hidden = true;

	esp_err_t rc = ::esp_wifi_scan_start(&conf, true);
	if (rc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_wifi_scan_start: %d", rc);
			return apRecords;
	}

	uint16_t apCount;  // Number of access points available.
	rc = ::esp_wifi_scan_get_ap_num(&apCount);
	ESP_LOGD(LOG_TAG, "Count of found access points: %d", apCount);

	wifi_ap_record_t* list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
	if (list == nullptr) {
		ESP_LOGE(LOG_TAG, "Failed to allocate memory");
		return apRecords;
	}

	errRc = ::esp_wifi_scan_get_ap_records(&apCount, list);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_wifi_scan_get_ap_records: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		abort();
	}

	for (auto i=0; i<apCount; i++) {
		WiFiAPRecord wifiAPRecord;
		memcpy(wifiAPRecord.m_bssid, list[i].bssid, 6);
		wifiAPRecord.m_ssid     = std::string((char *)list[i].ssid);
		wifiAPRecord.m_authMode = list[i].authmode;
		wifiAPRecord.m_rssi     = list[i].rssi;
		apRecords.push_back(wifiAPRecord);
	}
	free(list);   // Release the storage allocated to hold the records.
	std::sort(apRecords.begin(),
		apRecords.end(),
		[](const WiFiAPRecord& lhs,const WiFiAPRecord& rhs){ return lhs.m_rssi> rhs.m_rssi;});
	return apRecords;
*/
} // scan


/**
 * @brief Start being an access point.
 *
 * @param[in] ssid The SSID to use to advertize for stations.
 * @param[in] password The password to use for station connections.
 * @param[in] auth The authorization mode for access to this access point.  Options are:
 * * WIFI_AUTH_OPEN
 * * WIFI_AUTH_WPA_PSK
 * * WIFI_AUTH_WPA2_PSK
 * * WIFI_AUTH_WPA_WPA2_PSK
 * * WIFI_AUTH_WPA2_ENTERPRISE
 * * WIFI_AUTH_WEP
 * @return N/A.
 */
bool WiFi::startAP(const std::string& ssid, const std::string& password, wifi_auth_mode_t auth) {
	return startAP(ssid, password, auth, 0, false, 4);
} // startAP

/**
 * @brief Start being an access point.
 *
 * @param[in] ssid The SSID to use to advertize for stations.
 * @param[in] password The password to use for station connections.
 * @param[in] auth The authorization mode for access to this access point.  Options are:
 * * WIFI_AUTH_OPEN
 * * WIFI_AUTH_WPA_PSK
 * * WIFI_AUTH_WPA2_PSK
 * * WIFI_AUTH_WPA_WPA2_PSK
 * * WIFI_AUTH_WPA2_ENTERPRISE
 * * WIFI_AUTH_WEP
 * @param[in] channel from the access point.
 * @param[in] is the ssid hidden, ore not.
 * @param[in] limiting number of clients.
 * @return N/A.
 */
bool WiFi::startAP(const std::string& ssid, const std::string& password, wifi_auth_mode_t auth, uint8_t channel, bool ssid_hidden, uint8_t max_connection) {
 	bool bRetVal = false;
	ESP_LOGD(LOG_TAG, ">> startAP: ssid: %s", ssid.c_str());
	esp_err_t errRC = ::esp_wifi_set_mode(WIFI_MODE_AP);
	if(ESP_OK==errRC) {
		// Build the apConfig structure.
		wifi_config_t apConfig;
		::memset(&apConfig, 0, sizeof(apConfig));
		::memcpy(apConfig.ap.ssid, ssid.data(), ssid.size());
		apConfig.ap.ssid_len = ssid.size();
		::memcpy(apConfig.ap.password, password.data(), password.size());
		apConfig.ap.channel         = channel;
		apConfig.ap.authmode        = auth;
		apConfig.ap.ssid_hidden     = (uint8_t) ssid_hidden;
		apConfig.ap.max_connection  = max_connection;
		apConfig.ap.beacon_interval = 100;

      errRC=esp_wifi_set_config(WIFI_IF_AP, &apConfig);
		if(ESP_OK==errRC) {
      	errRC=esp_wifi_start();
			if(ESP_OK==errRC) {
	      	ESP_LOGI(LOG_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", ssid.c_str(), password.c_str(), channel);
				bRetVal = true;
			} else {
				ESP_LOGE(LOG_TAG, "esp_wifi_start: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
			}
		} else {
			ESP_LOGE(LOG_TAG, "esp_wifi_set_config: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
		}
	} else {
		ESP_LOGE(LOG_TAG, "esp_wifi_set_mode: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
	}
	ESP_LOGD(LOG_TAG, "<< startAP");
	return bRetVal;
} 

/**
 * @brief Set the event handler to use to process detected events.
 * @param[in] wifiEventHandler The class that will be used to process events.
 */
void WiFi::setWifiEventHandler(WiFiEventHandler* wifiEventHandler) {
	ESP_LOGI(LOG_TAG, ">> setWifiEventHandler: 0x%d", (uint32_t)wifiEventHandler);
	MyWiFiEventHandler = wifiEventHandler;
} 

WiFiEventHandler* WiFi::getWifiEventHandler() {
  	return MyWiFiEventHandler;
}

/**
 * @brief Return a string representation of the WiFi access point record.
 *
 * @return A string representation of the WiFi access point record.
 */
std::string WiFiAPRecord::toString() {
	std::string auth;
	switch(getAuthMode()) {
		case WIFI_AUTH_OPEN:
			auth = "WIFI_AUTH_OPEN";
			break;
		case WIFI_AUTH_WEP:
			auth = "WIFI_AUTH_WEP";
			break;
		case WIFI_AUTH_WPA_PSK:
			auth = "WIFI_AUTH_WPA_PSK";
			break;
		case WIFI_AUTH_WPA2_PSK:
			auth = "WIFI_AUTH_WPA2_PSK";
			break;
		case WIFI_AUTH_WPA_WPA2_PSK:
			auth = "WIFI_AUTH_WPA_WPA2_PSK";
			break;
    default:
        auth = "<unknown>";
        break;
    }
//    std::stringstream s;
//    s<< "ssid: " << m_ssid << ", auth: " << auth << ", rssi: " << m_rssi;
    auto info_str = (char*) malloc(6 + 32 + 8 + 22 + 8 + 3 + 1);
    sprintf(info_str, "ssid: %s, auth: %s, rssi: %d", m_ssid.c_str(), auth.c_str(), (int) m_rssi);
    return std::string(std::move(info_str));
} // toString

bool WiFi::scan(bool showHidden) {
/*
	if(!init()) {
		return false;
	}
	esp_wifi_set_mode(WIFI_MODE_STA);
	wifi_config_t wifi_config;
	memset(&wifi_config.sta,0,sizeof(wifi_sta_config_t));
	wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
	wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
	esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	esp_wifi_start();

	wifi_scan_config_t conf;
	memset(&conf, 0, sizeof(conf));
	conf.show_hidden = showHidden;

	return ESP_OK==::esp_wifi_scan_start(&conf, false);
*/
	return ESP_OK;
}

bool WiFi::stopWiFi() {
	return ESP_OK==esp_wifi_stop();
}

bool WiFi::shutdown() {
	return ESP_OK==esp_wifi_deinit();
}

