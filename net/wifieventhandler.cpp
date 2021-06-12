/*
 * WiFiEventHandler.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: kolban
 */

#include "wifieventhandler.h"
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>
#include "sdkconfig.h"

static const char* LOG_TAG = "WiFiEventHandler";
using namespace libesp;

/**
 * @brief The entry point into the event handler.
 * Examine the event passed into the handler controller by the WiFi subsystem and invoke
 * the corresponding handler.
 * @param [in] event
 * @return ESP_OK if the event was handled otherwise an error.
 */
esp_err_t WiFiEventHandler::eventHandler(esp_event_base_t event_base, int32_t event_id, void* event_data) {
	ErrorType rc;
	switch(event_id) {
		case WIFI_EVENT_WIFI_READY: { // ESP32 WiFi ready
			rc = wifiReady();
			break;
		}
		case WIFI_EVENT_SCAN_DONE: {  // ESP32 finish scanning AP
			wifi_event_sta_scan_done_t *event = (wifi_event_sta_scan_done_t *) event_data;
			rc = staScanDone(event);
			break;
		}
		case WIFI_EVENT_STA_START: // ESP32 station start
		{ 
			rc = staStart();
			break;
		}
		case WIFI_EVENT_STA_STOP: // ESP32 station stop
		{
			rc = staStop();
			break;
		}
		case WIFI_EVENT_STA_CONNECTED: // ESP32 station connected to AP
		{
			wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t*)event_data;
			rc = staConnected(event);
			break;
		}
		case WIFI_EVENT_STA_DISCONNECTED: // ESP32 station disconnected from AP
		{
			wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t*)event_data;
			rc = staDisconnected(event);
			break;
		}
		case WIFI_EVENT_STA_AUTHMODE_CHANGE: // the auth mode of AP connected by ESP32 station changed
		{
			wifi_event_sta_authmode_change_t *event = (wifi_event_sta_authmode_change_t*)event_data;
			rc = staAuthChange(event);
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_SUCCESS: // ESP32 station wps succeeds in enrollee mode 
		{
			wifi_event_sta_wps_er_success_t *event = (wifi_event_sta_wps_er_success_t*)event_data;
			rc = wpsERPinSuccess(event);
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_FAILED: // ESP32 station wps fails in enrollee mode 
		{
			wifi_event_sta_wps_fail_reason_t *event = (wifi_event_sta_wps_fail_reason_t*)event_data;
			rc = wpsERFailed(event);
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_TIMEOUT: // ESP32 station wps timeout in enrollee mode 
		{
			rc = staWPSERTimeout();
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_PIN: // ESP32 station wps pin code in enrollee mode
		{
			wifi_event_sta_wps_er_pin_t *event = (wifi_event_sta_wps_er_pin_t*)event_data;
			rc = wpsERPin(event);
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP: // ESP32 station wps overlap in enrollee mode
		{
			rc =  wpsPCBOverLap();
			break;
		}
		case WIFI_EVENT_AP_START: // ESP32 soft-AP start
		{
			rc =  apStart();
			break;
		}
		case WIFI_EVENT_AP_STOP: // ESP32 soft-AP stop 
		{
			rc = apStop();
			break;
		}
		case WIFI_EVENT_AP_STACONNECTED: // a station connected to ESP32 soft-AP 
		{
			wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
			rc = apStaConnected(event);
			break;
		}
		case WIFI_EVENT_AP_STADISCONNECTED: // a station disconnected from ESP32 soft-AP
		{
			wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
			rc = apStaDisconnected(event);
			break;
		}
		case WIFI_EVENT_AP_PROBEREQRECVED: // Receive probe request packet in soft-AP interface 
		{
			wifi_event_ap_probe_req_rx_t *event = (wifi_event_ap_probe_req_rx_t*)event_data;
			rc = apProbeReq(event);
			break;
		}
		case WIFI_EVENT_FTM_REPORT: // Receive report of FTM procedure
		{
			wifi_event_ftm_report_t *event = (wifi_event_ftm_report_t*)event_data;
			rc = ftmReport(event);
			break;
		}
		case WIFI_EVENT_STA_BSS_RSSI_LOW: // AP’s RSSI crossed configured threshold
		{
			wifi_event_bss_rssi_low_t *event = (wifi_event_bss_rssi_low_t*)event_data;
			rc = staBSSRSSILow(event);
			break;
		}
		case WIFI_EVENT_ACTION_TX_STATUS: // Status indication of Action Tx operation 
		{
			wifi_event_action_tx_status_t *event = (wifi_event_action_tx_status_t*)event_data;
			rc = actionTXStatus(event);
			break;
		}
		case WIFI_EVENT_ROC_DONE: // Remain-on-Channel operation complete 
		{
			wifi_event_roc_done_t *event = (wifi_event_roc_done_t*)event_data;
			rc = rocDone(event);
			break;
		}
		case WIFI_EVENT_STA_BEACON_TIMEOUT:
		{
			rc = staBeaconTimeout();
			break;
		}
		default:
			break;
    }

    return rc.getErrT();
}


/**
 * @brief Constructor
 */
WiFiEventHandler::WiFiEventHandler() {
} 


/**
 * @brief Handle the Station Got IP event.
 * Handle having received/assigned an IP address when we are a station.
 * @param [in] event_sta_got_ip The Station Got IP event.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::staGotIp(system_event_sta_got_ip_t info) {
    ESP_LOGD(LOG_TAG, "default staGotIp");
    return ESP_OK;
} // staGotIp


/**
 * @brief Handle the Access Point started event.
 * Handle an indication that the ESP32 has started being an access point.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::apStart() {
    ESP_LOGD(LOG_TAG, "default apStart");
    return ESP_OK;
} // apStart


/**
 * @brief Handle the Access Point stop event.
 * Handle an indication that the ESP32 has stopped being an access point.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::apStop() {
    ESP_LOGD(LOG_TAG, "default apStop");
    return ESP_OK;
} // apStop


ErrorType WiFiEventHandler::wifiReady() {
    ESP_LOGD(LOG_TAG, "default wifiReady");
    return ESP_OK;
} // wifiReady


ErrorType WiFiEventHandler::staStart() {
    ESP_LOGD(LOG_TAG, "default staStart");
    return ESP_OK;
} // staStart


ErrorType WiFiEventHandler::staStop() {
    ESP_LOGD(LOG_TAG, "default staStop");
    return ESP_OK;
} // staStop


ErrorType WiFiEventHandler::wpsERPinSuccess(wifi_event_sta_wps_er_success_t *info) {
    return ESP_OK;
}

ErrorType WiFiEventHandler::wpsERFailed(wifi_event_sta_wps_fail_reason_t *info) {
    return ESP_OK;
}

ErrorType WiFiEventHandler::wpsERPin(wifi_event_sta_wps_er_pin_t *info) {
	return ESP_OK;
}

ErrorType WiFiEventHandler::apProbeReq(wifi_event_ap_probe_req_rx_t *info) {
	return ESP_OK;
}

ErrorType WiFiEventHandler::ftmReport(wifi_event_ftm_report_t *info) {
	return ESP_OK;
}

ErrorType WiFiEventHandler::staBSSRSSILow(wifi_event_bss_rssi_low_t *info) {
	return ESP_OK;
}

ErrorType WiFiEventHandler::actionTXStatus(wifi_event_action_tx_status_t *info) {
	return ESP_OK;

}

ErrorType WiFiEventHandler::rocDone(wifi_event_roc_done_t *info) {
	return ESP_OK;
}

ErrorType WiFiEventHandler::staWPSERTimeout() {
	return ESP_OK;
}

ErrorType WiFiEventHandler::staBeaconTimeout() {
	return ESP_OK;
}

/**
 * @brief Handle the Station Connected event.
 * Handle having connected to remote AP.
 * @param [in] event_connected system_event_sta_connected_t.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::staConnected(system_event_sta_connected_t *info) {
	ESP_LOGD(LOG_TAG, "default staConnected");
	return ESP_OK;
} // staConnected


/**
 * @brief Handle the Station Disconnected event.
 * Handle having disconnected from remote AP.
 * @param [in] event_disconnected system_event_sta_disconnected_t.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::staDisconnected(system_event_sta_disconnected_t *info) {
    ESP_LOGD(LOG_TAG, "default staDisconnected");
    return ESP_OK;
} // staDisconnected


/**
 * @brief Handle a Station Connected to AP event.
 * Handle having a station connected to ESP32 soft-AP.
 * @param [in] event_sta_connected system_event_ap_staconnected_t.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::apStaConnected(wifi_event_ap_staconnected_t *info) {
	ESP_LOGI(LOG_TAG, "station "MACSTR" join, AID=%d", MAC2STR(info->mac), info->aid);
    return ESP_OK;
} // apStaConnected


/**
 * @brief Handle a Station Disconnected from AP event.
 * Handle having a station disconnected from ESP32 soft-AP.
 * @param [in] event_sta_disconnected system_event_ap_stadisconnected_t.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::apStaDisconnected(wifi_event_ap_stadisconnected_t *info) {
	ESP_LOGI(LOG_TAG, "station "MACSTR" join, AID=%d", MAC2STR(info->mac), info->aid);
    return ESP_OK;
} // apStaDisconnected


/**
 * @brief Handle a Scan for APs done event.
 * Handle having an ESP32 station scan (APs) done.
 * @param [in] event_scan_done system_event_sta_scan_done_t.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::staScanDone(system_event_sta_scan_done_t *info) {
    ESP_LOGD(LOG_TAG, "default staScanDone");
    return ESP_OK;
} // staScanDone


/**
 * @brief Handle the auth mode of APs change event.
 * Handle having the auth mode of AP ESP32 station connected to changed.
 * @param [in] event_auth_change system_event_sta_authmode_change_t.
 * @return An indication of whether or not we processed the event successfully.
 */
ErrorType WiFiEventHandler::staAuthChange(system_event_sta_authmode_change_t *info) {
    ESP_LOGD(LOG_TAG, "default staAuthChange");
    return ESP_OK;
} // staAuthChange

ErrorType WiFiEventHandler::wpsPCBOverLap() {
	return ESP_OK;
}

WiFiEventHandler::~WiFiEventHandler() {
} // ~WiFiEventHandler
