#pragma once

#include "../error_type.h"

namespace libesp {
//steps for OTA
// build new app
// deploy to web server

// in app_main
// validate basics are working (EG: check known pin voltages, etc if all is good call:
//    applyUpdate(true)
// if not: call applyUpdate(false) to rollback

class OTAProgress {
   public:
      enum PROGRESS {
         INIT
         , CONNECTED
         , HTTP_READ
         , HTTP_READ_ERROR
         , NO_NEW_VERSION_AVAIL
         , IMAGE_HEADER_CHECK_SUCCESSFUL
         , OTA_BEGIN_FAILED
         , OTA_WRITE_UPDATE_START
         , OTA_FAILED_WRITE
         , COMPLETE
         , PARITION_SWAP_COMPLETE
      };
   public:
      OTAProgress();
      void init(uint32_t startTimeMS);
      void updateProgress(const PROGRESS &p);
      virtual ~OTAProgress();
      void setSingleReadBytes(int32_t b) {BytesReadThisPass = b;}
      void setCurrentVersion(const char *cv);
      void setDownloadedVersion(const char *dv);
      int32_t getSingleReadBytes() const {return BytesReadThisPass;}
      const char *getCurrentVersion() const {return &CurrentVersion[0];}
      const char *getDownloadVersion() const {return &DownloadVersion[0];}
      void setTotalBytes(int32_t b) {TotalBytesRead = b;}
      int32_t getTotalBytes() const {return TotalBytesRead;}
   protected:
      virtual void onUpdate(const PROGRESS &p)=0;
   protected:
      int32_t BytesReadThisPass;
      int32_t TotalBytesRead;
      PROGRESS CurrentProgress;
      uint32_t StartTime;
      char CurrentVersion[33];
      char DownloadVersion[33];
};


class OTA {
   public:
      OTA();
      ErrorType init(const char *url);
      ErrorType logCurrentActiveParitionInfo();
      ErrorType run(OTAProgress *progressUpdate);
      ErrorType applyUpdate(bool validBoot);
      bool isUpdateAvailable() const {return UpdateAvailable;}
      const char *getCurrentApplicationVersion();
      const char *getBuildDate();
      const char *getBuildTime();
      virtual ~OTA();
   protected:
      int32_t BytesReadFromUpdate;
      bool UpdateAvailable;

};

}
