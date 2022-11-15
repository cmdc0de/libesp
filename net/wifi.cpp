/*
 * WiFi.cpp
 *
 */

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
#include "esp32inet.h"


using namespace libesp;
const char* WiFi::LOGTAG = "WiFi";

/**
 * @brief Creates and uses a default event handler
 */
WiFi::WiFi() : MyWiFiEventHandler(nullptr), WiFiMode(WIFI_MODE_NULL) {
} 


/**
 * @brief Deletes the event handler that was used by the class
 */
WiFi::~WiFi() {
} 


/**
 * STATIC
  * @brief Primary event handler interface.
  * typedefvoid (*esp_event_handler_t)(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data
 */
void WiFi::eventHandler(void* ctx, esp_event_base_t event_base, int32_t event_id, void *event_data) {

	WiFi *pWiFi = (WiFi *)ctx;   // retrieve the WiFi object from the passed in context.
	ESP_LOGI(LOGTAG, ">> WifiEventHandler: 0x%d", (uint32_t)pWiFi->MyWiFiEventHandler);

	if (pWiFi->MyWiFiEventHandler != nullptr) {
	  ESP_LOGI(LOGTAG, "calling event handler:  event_id %d, event_data %d", event_id, (uint32_t)event_data);
		pWiFi->MyWiFiEventHandler->eventHandler(event_base,event_id,event_data);
	}
} 

ErrorType WiFi::initAPSTA() {
  ErrorType et;
  if(ESP32INet::get()->createWifiInterfaceAP()!=nullptr && 
    ESP32INet::get()->createWifiInterfaceSTA()) {
	  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    et = esp_wifi_init(&cfg);
    ESP_LOGI(LOGTAG,"esp_wifi_init");
    if(et.ok()) {
      et = esp_wifi_set_mode(WIFI_MODE_APSTA);
      if(et.ok()) {
		    et = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandler, this);
		    et = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandler, this);
        if(et.ok()) {
          et = esp_wifi_set_storage(WIFI_STORAGE_RAM);
          if(et.ok()) {
            et = esp_wifi_start();
            if(!et.ok()) {
              ESP_LOGE(LOGTAG, "esp_wifi_start(): %u %s", et.getErrT(), et.toString());
            }
          } else {
            ESP_LOGE(LOGTAG, "esp storage: %u %s", et.getErrT(), et.toString());
          }
        } else {
          ESP_LOGE(LOGTAG, "esp_event handle: %u %s", et.getErrT(), et.toString());
        }
      } else {
        ESP_LOGE(LOGTAG, "esp set mode : %u %s", et.getErrT(), et.toString());
      }
    } else {
      ESP_LOGE(LOGTAG, "wifi init: %u %s", et.getErrT(), et.toString());
    }
  } else {
    ESP_LOGE(LOGTAG, "failed to create STA interface");
  }
  return et;
}

ErrorType WiFi::initAP() {
   ESP_LOGI(LOGTAG,"initAP");
  ErrorType et;
  if(ESP32INet::get()->createWifiInterfaceAP()!=nullptr) {
	  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    et = esp_wifi_init(&cfg);
    ESP_LOGI(LOGTAG,"esp_wifi_init");
    if(et.ok()) {
      et = esp_wifi_set_mode(WIFI_MODE_AP);
      if(et.ok()) {
		    et = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandler, this);
        if(et.ok()) {
          et = esp_wifi_set_storage(WIFI_STORAGE_RAM);
          if(et.ok()) {
            et = esp_wifi_start();
            if(!et.ok()) {
              ESP_LOGE(LOGTAG, "esp_wifi_start(): %u %s", et.getErrT(), et.toString());
            }
          } else {
            ESP_LOGE(LOGTAG, "esp storage: %u %s", et.getErrT(), et.toString());
          }
        } else {
          ESP_LOGE(LOGTAG, "esp_event handle: %u %s", et.getErrT(), et.toString());
        }
      } else {
        ESP_LOGE(LOGTAG, "esp set mode : %u %s", et.getErrT(), et.toString());
      }
    } else {
      ESP_LOGE(LOGTAG, "wifi init: %u %s", et.getErrT(), et.toString());
    }
  } else {
    ESP_LOGE(LOGTAG, "failed to create AP interface");
  }
  return et; 
}

ErrorType WiFi::initSTA() {
  ErrorType et;
  if(ESP32INet::get()->createWifiInterfaceSTA()!=nullptr) {
    ESP_LOGI(LOGTAG,"after wifi Interface STA");
	  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    et = esp_wifi_init(&cfg);
    ESP_LOGI(LOGTAG,"esp_wifi_init");
    if(et.ok()) {
      et = esp_wifi_set_mode(WIFI_MODE_STA);
      if(et.ok()) {
		    et = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandler, this);
        if(et.ok()) {
          et = esp_wifi_set_storage(WIFI_STORAGE_RAM);
          if(et.ok()) {
            et = esp_wifi_start();
            if(!et.ok()) {
              ESP_LOGE(LOGTAG, "esp_wifi_start(): %u %s", et.getErrT(), et.toString());
            }
          } else {
            ESP_LOGE(LOGTAG, "esp storage: %u %s", et.getErrT(), et.toString());
          }
        } else {
          ESP_LOGE(LOGTAG, "esp_event handle: %u %s", et.getErrT(), et.toString());
        }
      } else {
        ESP_LOGE(LOGTAG, "esp set mode : %u %s", et.getErrT(), et.toString());
      }
    } else {
      ESP_LOGE(LOGTAG, "wifi init: %u %s", et.getErrT(), et.toString());
    }
  } else {
    ESP_LOGE(LOGTAG, "failed to create STA interface");
  }
  return et;
}

ErrorType WiFi::connect(const SSIDTYPE &ssid, const PASSWDTYPE &pass, wifi_auth_mode_t mode) {
  ErrorType et;
  wifi_config_t conf;
  memset(&conf,0,sizeof(conf));
  strcpy(reinterpret_cast<char*>(&conf.sta.ssid[0]),ssid.c_str());
  strcpy(reinterpret_cast<char*>(&conf.sta.password[0]),pass.c_str());
  conf.sta.scan_method = WIFI_FAST_SCAN;
  conf.sta.threshold.authmode = mode;
  //esp_wifi_set_mode(WIFI_MODE_STA);
  et = esp_wifi_set_config(WIFI_IF_STA, &conf);
  if(et.ok()) {
    et = esp_wifi_connect();
  } else {
    ESP_LOGE(LOGTAG, "wifi set config: %u %s", et.getErrT(), et.toString());
  }
  return et;
}

ErrorType WiFi::setWiFiStorage(wifi_storage_t storage) {
  return ::esp_wifi_set_storage(storage);
}

/**
* @brief Initialize WiFi.
*/
ErrorType WiFi::init(const wifi_mode_t &wmode) {
  WiFiMode = wmode;
  ESP32INet::get();//to init inet
  ErrorType et;
  et = esp_event_loop_create_default();
  if(et.ok()) {
    switch(WiFiMode) {
    case WIFI_MODE_STA:
      et = initSTA();
      break;
    case WIFI_MODE_AP:
      et = initAP();
      break;
    case WIFI_MODE_APSTA:
      et = initAPSTA();
      break;
    default:
      ESP_LOGI(LOGTAG,"unknwon init mode");
    }
  } else {
      ESP_LOGE(LOGTAG,"failed to create default loop");
  }
   return et;
} 

ErrorType WiFi::scan(etl::vector<WiFiAPRecord,16> &results, bool showHidden) {
	wifi_scan_config_t conf;
	memset(&conf, 0, sizeof(conf));
	conf.show_hidden = showHidden;
  return scan(results,conf);
}

/**
 */
ErrorType WiFi::scan(etl::vector<WiFiAPRecord,16> &results,  const wifi_scan_config_t &conf) {
  //force blocking call
  ErrorType et =  ::esp_wifi_scan_start(&conf, true);
  if(et.ok()) {
	  uint16_t apCount;  // Number of access points available.
    et = ::esp_wifi_scan_get_ap_num(&apCount);
    if(et.ok()) {
      if(apCount>16) apCount = 16;
      ESP_LOGI(LOGTAG, "Count of found access points: %d", apCount);
	    wifi_ap_record_t list[16]; 
	    et = ::esp_wifi_scan_get_ap_records(&apCount, &list[0]);
      if(et.ok()) {
	      for (auto i=0; i<apCount; i++) {
      		WiFiAPRecord wifiAPRecord;
          memcpy(wifiAPRecord.m_bssid, list[i].bssid, 6);
          wifiAPRecord.m_ssid     = ((char *)list[i].ssid);
          wifiAPRecord.m_authMode = list[i].authmode;
          wifiAPRecord.m_rssi     = list[i].rssi;
          wifiAPRecord.setPrimaryChannel(list[i].primary);
          uint32_t flags = 0;
          if(list[i].phy_11b) flags|=1<<0;
          if(list[i].phy_11g) flags|=1<<1;
          if(list[i].phy_11n) flags|=1<<2;
          if(list[i].phy_lr) flags|=1<<3;
          if(list[i].wps) flags|=1<<4;
          if(list[i].ftm_responder) flags|=1<<5;
          if(list[i].ftm_initiator) flags|=1<<6;
          wifiAPRecord.setFlags(flags);
          results.push_back(wifiAPRecord);
	      }
	      std::sort(results.begin(), results.end(), [](const WiFiAPRecord& lhs,const WiFiAPRecord& rhs){ return lhs.m_rssi > rhs.m_rssi;});
      } else {
		    ESP_LOGE(LOGTAG, "esp_wifi_scan_get_ap_records: rc=%d %s", et.getErrT(), et.toString());
      }
    } else {
      ESP_LOGE(LOGTAG, "esp_wifi_scan_get_ap_num: %d %s", et.getErrT(), et.toString());
    }
  } else {
    ESP_LOGE(LOGTAG, "esp_wifi_scan_start: %d %s", et.getErrT(), et.toString());
	}
  return et;
}

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
	ESP_LOGD(LOGTAG, ">> startAP: ssid: %s", ssid.c_str());
	//esp_err_t errRC = ::esp_wifi_set_mode(WIFI_MODE_AP);
	esp_err_t errRC = ESP_OK;
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
	      	ESP_LOGI(LOGTAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", ssid.c_str(), password.c_str(), channel);
				bRetVal = true;
			} else {
				ESP_LOGE(LOGTAG, "esp_wifi_start: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
			}
		} else {
			ESP_LOGE(LOGTAG, "esp_wifi_set_config: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
		}
	} else {
		ESP_LOGE(LOGTAG, "esp_wifi_set_mode: rc=%d %s", errRC, GeneralUtils::errorToString(errRC));
	}
	ESP_LOGD(LOGTAG, "<< startAP");
	return bRetVal;
} 

/**
 * @brief Set the event handler to use to process detected events.
 * @param[in] wifiEventHandler The class that will be used to process events.
 */
void WiFi::setWifiEventHandler(WiFiEventHandler* wifiEventHandler) {
	ESP_LOGI(LOGTAG, ">> setWifiEventHandler: 0x%d", (uint32_t)wifiEventHandler);
	MyWiFiEventHandler = wifiEventHandler;
} 

WiFiEventHandler* WiFi::getWifiEventHandler() {
  	return MyWiFiEventHandler;
}

const char * WiFiAPRecord::getAuthModeString() {
	switch(getAuthMode()) {
		case WIFI_AUTH_OPEN:
			return "WIFI_AUTH_OPEN";
		case WIFI_AUTH_WEP:
			return "WIFI_AUTH_WEP";
		case WIFI_AUTH_WPA_PSK:
			return "WIFI_AUTH_WPA_PSK";
		case WIFI_AUTH_WPA2_PSK:
			return "WIFI_AUTH_WPA2_PSK";
		case WIFI_AUTH_WPA_WPA2_PSK:
			return "WIFI_AUTH_WPA_WPA2_PSK";
    default:
      return "<unknown>";
    }
}

/**
 * @brief Return a string representation of the WiFi access point record.
 * @return A string representation of the WiFi access point record.
 */
WiFiAPRecord::TO_STRING_TYPE WiFiAPRecord::toString() {
  char info_str[128] = {'\0'};
  sprintf(info_str, "ssid: %20s, auth: %24s, rssi: %4d chan: %d, B:%s N:%s G:%s LR:%s"
    ,m_ssid.c_str(), getAuthModeString(), (int) m_rssi, static_cast<int32_t>(mPrimaryChannel)
    ,isWirelessB()?"Y":"N", isWirelessN()?"Y":"N", isWirelessG()?"Y":"N", isWirelessLR()?"Y":"N");
  return TO_STRING_TYPE (&info_str[0]);
} 

bool WiFi::stopWiFi() {
   esp_event_loop_delete_default();
	return ESP_OK==esp_wifi_stop();
}

bool WiFi::shutdown() {
	return ESP_OK==esp_wifi_deinit();
}

