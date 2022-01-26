
#include "networktimeprotocol.h"
#include <string.h>
#include <time.h>

using namespace libesp;

const char *NTP::LOGTAG = "NTP";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(NTP::LOGTAG, "Notification of a time synchronization event");
}

NTP::NTP() {

}

const char *ntpServerIDs[] = {
  "ntp1"
  , "ntp2"
  , "ntp3"
};

char ntpServers[3][32] = {
    "0.us.pool.ntp.org"
  , "1.us.pool.ntp.org"
  , "2.us.pool.ntp.org"
};


ErrorType NTP::init(NVS &storage, bool bSmooth, sntp_sync_time_cb_t cb) {
  ErrorType et;
  sntp_servermode_dhcp(1);
  //check storage for dns
  for(int i = 0;i<3;++i) {
    uint32_t len = 32;
    if(!storage.getValue(ntpServerIDs[i],&ntpServers[i][0],len).ok()) {
      break;
    }
  }
  if(cb==nullptr) {
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  } else {
    sntp_set_time_sync_notification_cb(cb);
  }

  sntp_setoperatingmode(SNTP_OPMODE_POLL);

  int maxNTPServers = SNTP_MAX_SERVERS>3 ? 3 : SNTP_MAX_SERVERS;

  for(int i=0;i<maxNTPServers;++i) {
    sntp_setservername(i,&ntpServers[i][0]);
  }
  if(bSmooth) {
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
  } else {
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
  }

  for (uint8_t i = 0; i < maxNTPServers; ++i){
    if (sntp_getservername(i)) {
      ESP_LOGI(LOGTAG, "server %d: %s", i, sntp_getservername(i));
    } else {
      // we have either IPv4 or IPv6 address, let's print it
      char buff[INET6_ADDRSTRLEN];
      ip_addr_t const *ip = sntp_getserver(i);
      if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL) ESP_LOGI(LOGTAG, "server %d: %s", i, buff);
    }
  }
  return et;
}

ErrorType NTP::setNTPServer(NVS &storage, uint8_t num, const char *ntpServer) {
  int maxNTPServers = SNTP_MAX_SERVERS>3 ? 3 : SNTP_MAX_SERVERS;
  if(num>maxNTPServers) {
    return ErrorType(ErrorType::LIBESP_ERRORS::INVALID_PARAM);
  }
  return storage.setValue(ntpServerIDs[num],&ntpServers[num][0]);
}

ErrorType NTP::start() {
  ErrorType et;
  sntp_init();
  // wait for time to be set
  int retry = 0;
  const int retry_count = 15;
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    ESP_LOGI(LOGTAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  time_t now = 0;
  time(&now);
  struct tm timeinfo;
  memset(&timeinfo,0,sizeof(timeinfo));
  localtime_r(&now, &timeinfo);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(LOGTAG, "The current date/time : %s", strftime_buf);
  return et;
}

ErrorType NTP::stop() {
  ErrorType et;
  sntp_stop();
  return et;
}

NTP::~NTP() {

}
