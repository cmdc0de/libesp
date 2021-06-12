#include "esp32inet.h"
#include <esp_log.h>
#include <esp_wifi_default.h>

using namespace libesp;


ESP32INet * ESP32INet::mSelf = 0;
const char *ESP32INet::LOGTAG = "ESP32INET";

ESP32INet *ESP32INet::get() {
	if(0==mSelf) {
		mSelf = new ESP32INet();
	} 
	return mSelf;
}

ESP32INet::ESP32INet() : APInterface(nullptr), STAInterface(nullptr) {

}

ESP32INet::~ESP32INet() {
	shutdown();
}


esp_netif_t *ESP32INet::createWifiInterfaceAP() {
	if(nullptr==APInterface) {
		APInterface = esp_netif_create_default_wifi_ap();
		if(ESP_OK!=esp_netif_dhcps_start(APInterface)) {
			ESP_LOGE(LOGTAG,"Failed to start dhcp server");
		}
	}
	return APInterface;
}

esp_netif_t *ESP32INet::createWifiInterfaceSTA() {
	if(nullptr==STAInterface) {
		STAInterface = esp_netif_create_default_wifi_sta();
	}
	return STAInterface;
}

ErrorType ESP32INet::init() const {
	ErrorType retVal;
	retVal = esp_netif_init();
	return retVal;
}


ErrorType ESP32INet::shutdown() const {
	ErrorType retVal;
	retVal = esp_netif_deinit();
	return retVal;
}

/*
typedef enum esp_netif_flags {
    ESP_NETIF_DHCP_CLIENT = 1 << 0,
    ESP_NETIF_DHCP_SERVER = 1 << 1,
    ESP_NETIF_FLAG_AUTOUP = 1 << 2,
    ESP_NETIF_FLAG_GARP   = 1 << 3,
    ESP_NETIF_FLAG_EVENT_IP_MODIFIED = 1 << 4,
    ESP_NETIF_FLAG_IS_PPP = 1 << 5,
    ESP_NETIF_FLAG_IS_SLIP = 1 << 6,
} esp_netif_flags_t;
*/

//static
void ESP32INet::dumpToLog()  {
	size_t numInterfaces = esp_netif_get_nr_of_ifs();
	ESP_LOGI(LOGTAG,"There are %u interfaces",numInterfaces);
	esp_netif_t *netif = esp_netif_next(nullptr);
	for(uint32_t i=0;i<numInterfaces && netif!=0;i++) {
		ESP_LOGI(LOGTAG,"Interface: %u", i);
		ESP_LOGI(LOGTAG,"Description %s", esp_netif_get_desc(netif));
		ESP_LOGI(LOGTAG,"Route Priority %d", esp_netif_get_route_prio(netif));
		ESP_LOGI(LOGTAG,"Interface Key %s", esp_netif_get_ifkey(netif));
		ESP_LOGI(LOGTAG,"Flags: %u", esp_netif_get_flags(netif));
		//dns info
		esp_netif_dns_info_t dnsInfo;
		char buffer[24] = {'\0'};
		esp_err_t err = esp_netif_get_dns_info(netif,ESP_NETIF_DNS_MAIN,&dnsInfo);
		if(ESP_OK==err) {
			ESP_LOGI(LOGTAG,"Main DNS: %s", esp_ip4addr_ntoa(&(dnsInfo.ip.u_addr.ip4),&buffer[0],sizeof(buffer)));
		} else {
			ESP_LOGI(LOGTAG,"Error getting main DNS: %d",err);
		}
		err = esp_netif_get_dns_info(netif,ESP_NETIF_DNS_BACKUP,&dnsInfo);
		if(ESP_OK==err) {
			ESP_LOGI(LOGTAG,"Backup DNS: %s", esp_ip4addr_ntoa(&dnsInfo.ip.u_addr.ip4,&buffer[0],sizeof(buffer)));
		} else {
			ESP_LOGI(LOGTAG,"Error getting backup DNS: %d",err);
		}
		err = esp_netif_get_dns_info(netif,ESP_NETIF_DNS_FALLBACK,&dnsInfo);
		if(ESP_OK==err) {
			ESP_LOGI(LOGTAG,"Fallback DNS: %s", esp_ip4addr_ntoa(&dnsInfo.ip.u_addr.ip4,&buffer[0],sizeof(buffer)));
		} else {
			ESP_LOGI(LOGTAG,"Error getting fallback DNS: %d",err);
		}

		esp_netif_dhcp_status_t dhcpStatus;
		err = esp_netif_dhcps_get_status(netif,&dhcpStatus);
		if(ESP_OK==err) {
			ESP_LOGI(LOGTAG,"DHCP Server Status: %s", dhcpStatus==ESP_NETIF_DHCP_INIT?"INIT":dhcpStatus==ESP_NETIF_DHCP_STARTED?"Started":"Stopped");
		} else {
			ESP_LOGI(LOGTAG,"Error Getting dhcp server status: %d", err);
		}
		err = esp_netif_dhcpc_get_status(netif,&dhcpStatus);
		if(ESP_OK==err) {
			ESP_LOGI(LOGTAG,"DHCP Client Status: %s", dhcpStatus==ESP_NETIF_DHCP_INIT?"INIT":dhcpStatus==ESP_NETIF_DHCP_STARTED?"Started":"Stopped");
		} else {
			ESP_LOGI(LOGTAG,"Error Getting dhcp client status: %d", err);
		}
		err = esp_netif_get_netif_impl_name(netif,&buffer[0]);
		if(ESP_OK==err) {
			ESP_LOGI(LOGTAG,"NetIF Name: %s",&buffer[0]);
		} else {
			ESP_LOGI(LOGTAG,"Error Getting NetIf Name: %d", err);
		}

		esp_netif_ip_info_t espIP;
		err = esp_netif_get_ip_info(netif,&espIP);
		if(ESP_OK==err) {
			ESP_LOGI(LOGTAG,"IP Address: %s", esp_ip4addr_ntoa(&espIP.ip,&buffer[0],sizeof(buffer)));
			ESP_LOGI(LOGTAG,"NetMask: %s", esp_ip4addr_ntoa(&espIP.netmask,&buffer[0],sizeof(buffer)));
			ESP_LOGI(LOGTAG,"gatway: %s", esp_ip4addr_ntoa(&espIP.gw,&buffer[0],sizeof(buffer)));
		} else {
			ESP_LOGI(LOGTAG, "Error getting ip info %d", err);
		}
		uint8_t mac[16];
		err = esp_netif_get_mac(netif,&mac[0]);
		if(ESP_OK==err) {
			ESP_LOGI(LOGTAG,"Mac Address: ");
			ESP_LOG_BUFFER_HEX(LOGTAG,&mac[0],sizeof(mac));
		} else {
			ESP_LOGI(LOGTAG,"Error getting mac address: %d", err);
		}
		netif = esp_netif_next(netif);
	}
}

