#ifndef LIBESP_NETWORK_TIME_PROTOCOL_H
#define LIBESP_NETWORK_TIME_PROTOCOL_H

#pragma once

#include <esp_sntp.h>
#include "../nvs_memory.h"
#include "../error_type.h"

namespace libesp {

class NTP {
  public:
    static const char *LOGTAG;
  public:
    NTP();
    ErrorType init(NVS &storage, bool bSmooth, sntp_sync_time_cb_t cb);
    ErrorType setNTPServer(NVS &storage, uint8_t num, const char *ntpServer);
    ErrorType start();
    ErrorType stop();
    ~NTP();
  private:
};

}
#endif
