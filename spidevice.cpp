#include <string.h>
#include "driver/spi_common.h"
#include "spidevice.h"
#include "error_type.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "system.h"
#include <esp_log.h>
#include "locker.h"

using namespace libesp;

//base spi device

SPIDevice::SPIDevice(SPIBus *b, const spi_device_handle_t &sdh, const spi_device_interface_config_t &devcfg) : MyBus(b), SPIHandle(sdh), DevCfg(devcfg) {

}

bool SPIDevice::init() {
	return onInit();
}
	
ErrorType SPIDevice::shutdown() {
	return onShutdown();
}

ErrorType SPIDevice::sendAndReceive(uint8_t out, uint8_t &in) {
	return onSendAndReceive(out,in);
}

ErrorType SPIDevice::sendAndReceive(uint8_t *p, uint16_t len) {
	return onSendAndReceive(p,len);
}

ErrorType SPIDevice::sendAndReceive(uint8_t *out, uint8_t *in, uint16_t len) {
	return onSendAndReceive(out,in,len, nullptr);
}

ErrorType SPIDevice::sendAndReceive(uint8_t *out, uint8_t *in, uint16_t len, void *userData) {
	return onSendAndReceive(out,in,len, userData);
}

ErrorType SPIDevice::send(const uint8_t *p, uint16_t len) {
	return send(p,len, nullptr);
}

ErrorType SPIDevice::send(const uint8_t *p, uint16_t len, void *userData) {
	return onSend(p,len, userData);
}

SPIDevice::~SPIDevice() {

}


////////////////////////////////
//master
const char *SPIMaster::LOGTAG = "SPIMASTER";

static const int32_t MAX_DMA_LEN = SPI_MAX_DMA_LEN;

ErrorType SPIMaster::onSendAndReceive(uint8_t out, uint8_t &in) {
	//ESP_LOGI(LOGTAG,"send and receive same buffer 1 byte");
  ErrorType et;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));   //Zero out the transaction
	t.length=8;                 //Len is in bytes, transaction length is in bits.
	t.tx_data[0]=out;             //Data
	t.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
	//ESP_LOGI(LOGTAG,"before spi transmit");
  {
    MutexLocker ml(MySemaphore, MillisToWait);
    if(ml.take()) {
    	et  = spi_device_transmit(SPIHandle, &t);  //Transmit!
  	  in = t.rx_data[0];
    } else {
      et = ErrorType::TIMEOUT_ERROR;
    }
  }
	if (!et.ok()) {
		ESP_LOGE(LOGTAG,"%s", et.toString());
	}
	return et;
}

ErrorType SPIMaster::onSendAndReceive(uint8_t *p, uint16_t len) {
   //ESP_LOGI(LOGTAG,"send and receive same buffer");
   ErrorType et;
	if (len==0) return ESP_OK;       //no need to send anything

   int offset =0;
   do {
      spi_transaction_t t;
      memset(&t, 0, sizeof(t));   //Zero out the transaction
      
      int tx_len = ((len - offset) < MAX_DMA_LEN) ? (len - offset) : MAX_DMA_LEN-1;
      t.length=tx_len * 8;                       //Len is in bytes, transaction length is in bits.
      t.tx_buffer= p + offset;             //Data
      t.rx_buffer= p + offset;             //Data
      //esp_err_t r = spi_device_polling_transmit(SPIHandle, &t);  //Transmit!
      {
         MutexLocker ml(MySemaphore, MillisToWait);
         if(ml.take()) {
            et = spi_device_transmit(SPIHandle, &t);  //Transmit!
            if(!et.ok()) {
               break;
            }
         } else {
            et = ErrorType::TIMEOUT_ERROR;
            break;
         }
      }
      offset += tx_len;                          // Add the transmission length to the offset
   } while (offset < len);
	if (!et.ok()) {
		ESP_LOGE(LOGTAG,"%s", et.toString());
	}
	return et;
}

ErrorType SPIMaster::onSendAndReceive(uint8_t *out, uint8_t *in,uint16_t len, void *userData) {
	//ESP_LOGI(LOGTAG,"send and receive");
   ErrorType et;
	if (len==0) return ESP_OK;       //no need to send anything
                                    
   int offset = 0;
   do {
      spi_transaction_t t;
      memset(&t, 0, sizeof(t));   //Zero out the transaction
      
      int tx_len = ((len - offset) < MAX_DMA_LEN) ? (len - offset) : MAX_DMA_LEN-1;
      t.length=tx_len * 8;                       //Len is in bytes, transaction length is in bits.
      t.tx_buffer= out + offset;                //Data
	   t.rx_buffer= in + offset;             //Data
	   t.user = userData;
      {
         MutexLocker ml(MySemaphore, MillisToWait);
         if(ml.take()) {
            et = spi_device_transmit(SPIHandle, &t);  //Transmit!
            if(!et.ok()) {
               break;
            }
         } else {
            et = ErrorType::TIMEOUT_ERROR;
            break;
         }
      }
      offset += tx_len;                          // Add the transmission length to the offset
   } while (offset < len);
   if (!et.ok()) {
      ESP_LOGE(LOGTAG,"%s", et.toString());
	}
	return et;
}

ErrorType SPIMaster::onSend(const uint8_t *p, uint16_t len, void *userData) {
   //ESP_LOGI(LOGTAG,"send with userdata");
   ErrorType et;
	if (len==0) return ESP_OK;       //no need to send anything

   /*
   * On certain MC's the max SPI DMA transfer length might be smaller than the buffer. 
   * We then have to split the transmissions.
   */
   int offset = 0;
   do {
      spi_transaction_t t;
      memset(&t, 0, sizeof(t));       //Zero out the transaction

      int tx_len = ((len - offset) < MAX_DMA_LEN) ? (len - offset) : MAX_DMA_LEN-1;
      //ESP_LOGI(LOGTAG, "tx_len=%d max = %d", tx_len, MAX_DMA_LEN);
      t.length=tx_len * 8;                       //Len is in bytes, transaction length is in bits.
      t.tx_buffer= p + offset;                //Data
      t.user=userData;
      {
         MutexLocker ml(MySemaphore, MillisToWait);
         if(ml.take()) {
            //et=spi_device_polling_transmit(spi, &t);  //Transmit!
            et = spi_device_transmit(SPIHandle, &t);  //Transmit!
            if(!et.ok()) {
               break;
            }
         } else {
            et = ErrorType::TIMEOUT_ERROR;
            break;
         }
         offset += tx_len;                          // Add the transmission length to the offset
      }
   } while (offset < len);
   return et;
}

SPIMaster::~SPIMaster() {
	shutdown();
}

ErrorType SPIMaster::onShutdown() {
	return getBus()->removeDevice(this);
}

SPIMaster::SPIMaster(SPIBus* s, const spi_device_handle_t &sdh, const spi_device_interface_config_t &devcfg, SemaphoreHandle_t sem)
	: SPIDevice(s,sdh,devcfg), MySemaphore(sem), MillisToWait(MILLIS_DEFAULT_WAIT) {
	
}

bool SPIMaster::onInit() {
	return true;
}

