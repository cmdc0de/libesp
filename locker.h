#pragma once

#include <freertos/semphr.h>
#include <esp_log.h>

namespace libesp {

class MutexLocker {
  public:
    MutexLocker(SemaphoreHandle_t sh, uint32_t time) : Handle(sh), TotalTime(time*portTICK_PERIOD_MS) { 
      //ESP_LOGI("LOCKER", "handle is null %s", Handle==nullptr?"Y":"N");
    }
    bool take() {
      if(nullptr!=Handle)
        return pdTRUE==xSemaphoreTake(Handle,TotalTime);
      return true;
    }
    bool give() {
      if(nullptr!=Handle)
        return xSemaphoreGive(Handle)==pdTRUE;
      return true;
    }
    ~MutexLocker() {
      give();
    }
  private:
    SemaphoreHandle_t Handle;
    TickType_t TotalTime;
};

}
