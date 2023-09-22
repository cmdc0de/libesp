#include "ota.h"
#include <string.h>
#include "error_type.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "../freertos.h"
#include "esp_crt_bundle.h"

using namespace libesp;

#define HASH_LEN 32
char UpdateURL[256];


const char *TAG = "OTA";


//OTAProgress
OTAProgress::OTAProgress() : BytesReadThisPass(0), TotalBytesRead(0), CurrentProgress(INIT), StartTime(0),
   CurrentVersion(), DownloadVersion() {
}

void OTAProgress::init(uint32_t startTimeMS) {
   StartTime = startTimeMS;
}

void OTAProgress::updateProgress(const PROGRESS &p) {
   CurrentProgress = p;
   onUpdate(p);
}

void OTAProgress::setCurrentVersion(const char *cv) {
   strcpy(&CurrentVersion[0],cv);
}

void OTAProgress::setDownloadedVersion(const char *dv) {
   strcpy(&DownloadVersion[0],dv);
}

OTAProgress::~OTAProgress() {

}


static void http_cleanup(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void print_sha256 (const uint8_t *image_hash, const char *label) {
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

OTA::OTA() : BytesReadFromUpdate(-1), UpdateAvailable(false) {

}

static esp_app_desc_t running_app_info;

ErrorType OTA::init(const char *url) {
   ErrorType et;
   strcpy(&UpdateURL[0],url);
   ESP_LOGI(TAG,"Update URL set to: %s", &UpdateURL[0]);
   UpdateAvailable = false;
   BytesReadFromUpdate = -1;
   const esp_partition_t *running = esp_ota_get_running_partition();
   if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
      ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
   }
   esp_ota_img_states_t ota_state;
   et = esp_ota_get_state_partition(running, &ota_state);
   if (et.ok()) {
      if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
         UpdateAvailable = true;
      }
   }
   return et;
}

const char *OTA::getCurrentApplicationVersion() {
   return running_app_info.version;
}

const char *OTA::getBuildDate() {
   return running_app_info.date;
}

const char *OTA::getBuildTime() {
   return running_app_info.time;
}

ErrorType OTA::logCurrentActiveParitionInfo() {
   ErrorType et;
   uint8_t sha_256[HASH_LEN] = { 0 };
   esp_partition_t partition;
   const esp_partition_t *configured = esp_ota_get_boot_partition();
   const esp_partition_t *running = esp_ota_get_running_partition();

   if (configured != running) {
      ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
         configured->address, running->address);
      ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
      esp_partition_get_sha256(configured, sha_256);
      print_sha256(sha_256, "SHA-256 for configured firmware: ");
   }
   ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

   // get sha256 digest for running partition
   esp_partition_get_sha256(running, sha_256);
   print_sha256(sha_256, "SHA-256 for current firmware: ");


   // get sha256 digest for the partition table
   partition.address   = ESP_PARTITION_TABLE_OFFSET;
   partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
   partition.type      = ESP_PARTITION_TYPE_DATA;
   esp_partition_get_sha256(&partition, sha_256);
   print_sha256(sha_256, "SHA-256 for the partition table: ");

   // get sha256 digest for bootloader
   partition.address   = ESP_BOOTLOADER_OFFSET;
   partition.size      = ESP_PARTITION_TABLE_OFFSET;
   partition.type      = ESP_PARTITION_TYPE_APP;
   esp_partition_get_sha256(&partition, sha_256);
   print_sha256(sha_256, "SHA-256 for bootloader: ");

   ESP_LOGI(TAG, "Running App Info:");
   ESP_LOGI(TAG,"Project Name: %s", running_app_info.project_name);
   ESP_LOGI(TAG,"Version %s", running_app_info.version);
   ESP_LOGI(TAG,"Build date: %s", running_app_info.date);
   ESP_LOGI(TAG,"Build time: %s", running_app_info.time);

   return et;
}


ErrorType OTA::run(OTAProgress *progressUpdate) {
   ErrorType et;
   static constexpr const uint32_t BUFFSIZE = 4096;
   esp_ota_handle_t update_handle = 0 ;
   const esp_partition_t *update_partition = NULL;
   static char ota_write_data[BUFFSIZE+1] = { 0 };

   progressUpdate->init(FreeRTOS::getTimeSinceStart());

   esp_http_client_config_t config;
   memset(&config,0,sizeof(config));
   config.url = &UpdateURL[0];
   config.timeout_ms = 10000;
   config.keep_alive_enable = true;
   config.skip_cert_common_name_check = true;
   config.transport_type = HTTP_TRANSPORT_OVER_SSL;
   config.crt_bundle_attach = esp_crt_bundle_attach;

   esp_http_client_handle_t client = esp_http_client_init(&config);
   if (client == NULL) {
      ESP_LOGE(TAG, "Failed to initialise HTTP connection");
      return et;
   } else {
      ESP_LOGI(TAG,"client http initialized");
   }

   esp_http_client_set_url(client, &UpdateURL[0]);

   et = esp_http_client_open(client, 0);
   if (!et.ok()) {
      ESP_LOGE(TAG, "Failed to open HTTP connection: %s", et.toString());
      esp_http_client_cleanup(client);
      return et;
   } else {
      ESP_LOGI(TAG,"Connection established");
   }
   progressUpdate->updateProgress(OTAProgress::PROGRESS::CONNECTED);

   esp_http_client_fetch_headers(client);
   update_partition = esp_ota_get_next_update_partition(NULL);
   ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x"
         , update_partition->subtype, update_partition->address);

    BytesReadFromUpdate = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1) {
      int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
      progressUpdate->setSingleReadBytes(data_read);
      if (data_read < 0) {
         ESP_LOGE(TAG, "Error: SSL data read error");
         http_cleanup(client);
         et = esp_http_client_get_errno(client);
         progressUpdate->updateProgress(OTAProgress::PROGRESS::HTTP_READ_ERROR);
         return et;
      } else if (data_read > 0) {
         progressUpdate->updateProgress(OTAProgress::PROGRESS::HTTP_READ);
         if (image_header_was_checked == false) {
            progressUpdate->updateProgress(OTAProgress::PROGRESS::IMAGE_HEADER_CHECK_SUCCESSFUL);
            esp_app_desc_t new_app_info;
            if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) 
                  + sizeof(esp_app_desc_t)) {
               // check current version with downloading
               memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) 
                     + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
               ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);
               progressUpdate->setDownloadedVersion(new_app_info.version);

               //if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
               //   ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
               //}
               progressUpdate->setCurrentVersion(running_app_info.version);

               const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
               esp_app_desc_t invalid_app_info;
               if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                  ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
               }

               // check current version with last invalid partition
               if (last_invalid_app != NULL) {
                  if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                     ESP_LOGW(TAG, "New version is the same as invalid version.");
                     ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                     ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                     http_cleanup(client);
                     et = ErrorType::OTA_PREVIOUSLY_ROLLED_BACK;
                     return et;
                  }
               }
               if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                  ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                  http_cleanup(client);
                  progressUpdate->updateProgress(OTAProgress::PROGRESS::NO_NEW_VERSION_AVAIL);
                  et = ErrorType::OTA_NO_NEW_VERSION;
                  return et;
               }

               image_header_was_checked = true;
               progressUpdate->updateProgress(OTAProgress::PROGRESS::IMAGE_HEADER_CHECK_SUCCESSFUL);

               et = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
               if (!et.ok()) {
                  ESP_LOGE(TAG, "esp_ota_begin failed (%s)", et.toString());
                  http_cleanup(client);
                  esp_ota_abort(update_handle);
                  progressUpdate->updateProgress(OTAProgress::PROGRESS::OTA_BEGIN_FAILED);
                  return et;
               }
               ESP_LOGI(TAG, "esp_ota_begin succeeded");
               progressUpdate->updateProgress(OTAProgress::PROGRESS::OTA_WRITE_UPDATE_START);
            } else {
               ESP_LOGE(TAG, "received data is not large enough");
               http_cleanup(client);
               esp_ota_abort(update_handle);
               return et; //TODO FIX this is wrong its not an error to receive not enough...
            }
         }
         
         et = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
         if (!et.ok()) {
            http_cleanup(client);
            esp_ota_abort(update_handle);
            progressUpdate->updateProgress(OTAProgress::PROGRESS::OTA_FAILED_WRITE);
            return et;
         } 
         BytesReadFromUpdate += data_read;
         progressUpdate->setTotalBytes(BytesReadFromUpdate);
         ESP_LOGD(TAG, "Written image length %d", BytesReadFromUpdate);
      } else if (data_read == 0) {
         /*
         * As esp_http_client_read never returns negative error code, we rely on
         * `errno` to check for underlying transport connectivity closure if any
         */
         if (errno == ECONNRESET || errno == ENOTCONN) {
            ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
            break;
         }
         if (esp_http_client_is_complete_data_received(client) == true) {
            ESP_LOGI(TAG, "Connection closed");
            break;
         }
      }
   }
   ESP_LOGI(TAG, "Total Write binary data length: %d", BytesReadFromUpdate);
   if (esp_http_client_is_complete_data_received(client) != true) {
      ESP_LOGE(TAG, "Error in receiving complete file");
      http_cleanup(client);
      esp_ota_abort(update_handle);
      return ErrorType::INCOMPLETE_HTTP_GET;
   }

   et = esp_ota_end(update_handle);
   if (!et.ok()) {
      if (et.getErrT() == ESP_ERR_OTA_VALIDATE_FAILED) {
         ESP_LOGE(TAG, "Image validation failed, image is corrupted");
      } else {
         ESP_LOGE(TAG, "esp_ota_end failed (%s)!", et.toString());
      }
      http_cleanup(client);
      return et;
   }

   progressUpdate->updateProgress(OTAProgress::PROGRESS::COMPLETE);

   et = esp_ota_set_boot_partition(update_partition);
   if (!et.ok()) {
      ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", et.toString());
      http_cleanup(client);
      return et;
   }
   ESP_LOGI(TAG, "Prepare to restart system!");
   progressUpdate->updateProgress(OTAProgress::PROGRESS::PARITION_SWAP_COMPLETE);
   esp_restart();
}

ErrorType OTA::applyUpdate(bool validBoot) {
   ErrorType et;
   const esp_partition_t *running = esp_ota_get_running_partition();
   esp_ota_img_states_t ota_state;
   if (( et = esp_ota_get_state_partition(running, &ota_state)).ok()) {
      if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
         // run diagnostic function ...
         if (validBoot) {
            ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
            et = esp_ota_mark_app_valid_cancel_rollback();
         } else {
            ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
            et = esp_ota_mark_app_invalid_rollback_and_reboot();
         }
      }
   }
   return et;
}



OTA::~OTA() {

}



