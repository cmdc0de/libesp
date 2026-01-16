#ifndef LIBESP_WIFIEVENTHANDLER_H_
#define LIBESP_WIFIEVENTHANDLER_H_
#include <esp_event.h>
#include <esp_wifi_types.h>
#include <esp_netif_types.h>
#include "../error_type.h"

namespace libesp {
/*
 *
 * A WiFiEventHandler defines a class that has methods that will be called back when a WiFi event is
 * detected.
 *
 * Typically this class is subclassed to provide implementations for the callbacks we want to handle.
 *
 * class MyHandler: public WiFiEventHandler {
 *   ErrorType apStart() {
 *      ESP_LOGD(tag, "MyHandler(Class): apStart");
 *      return ESP_OK;
 *   }
 * }
 *
 * WiFi wifi;
 * MyHandler *eventHandler = new MyHandler();
 * wifi.setWifiEventHandler(eventHandler);
 * wifi.startAP("MYSSID", "password");
 *
 */
class WiFiEventHandler {
public:
	WiFiEventHandler();
	virtual ~WiFiEventHandler();
	virtual ErrorType apStaConnected(wifi_event_ap_staconnected_t *info);
	virtual ErrorType apStaDisconnected(wifi_event_ap_stadisconnected_t *info);
	virtual ErrorType apStart();
	virtual ErrorType apStop();
	virtual ErrorType staConnected(wifi_event_sta_connected_t *info);
	virtual ErrorType staDisconnected(wifi_event_sta_disconnected_t *info);
	virtual ErrorType staGotIp(ip_event_got_ip_t *info);
	virtual ErrorType staScanDone(wifi_event_sta_scan_done_t *info);
	virtual ErrorType staAuthChange(wifi_event_sta_authmode_change_t *info);
	virtual ErrorType wpsERPinSuccess(wifi_event_sta_wps_er_success_t *info);
	virtual ErrorType wpsERFailed(wifi_event_sta_wps_fail_reason_t *info);
	virtual ErrorType wpsERPin(wifi_event_sta_wps_er_pin_t *info);
	virtual ErrorType apProbeReq(wifi_event_ap_probe_req_rx_t *info);
	virtual ErrorType ftmReport(::wifi_event_ftm_report_t *info);
	virtual ErrorType staBSSRSSILow(wifi_event_bss_rssi_low_t *info);
	virtual ErrorType actionTXStatus(wifi_event_action_tx_status_t *info);
	virtual ErrorType rocDone(wifi_event_roc_done_t *info);
   virtual ErrorType staWPSERTimeout();
   virtual ErrorType staBeaconTimeout();
	virtual ErrorType wpsPCBOverLap();
	virtual ErrorType staStart();
	virtual ErrorType staStop();
	virtual ErrorType wifiReady();
	esp_err_t eventHandler(esp_event_base_t event_base, int32_t event_id, void* event_data);
};

}
#endif 
