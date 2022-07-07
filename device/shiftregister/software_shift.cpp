
#include "software_shift.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"
#include "../../system.h"
#include <string.h>

using namespace libesp;

const char *SoftwareShiftRegister::LOGTAG = "SoftwareShiftRegister";
static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[SoftwareShiftRegister::QUEUE_SIZE*SoftwareShiftRegister::MSG_SIZE] = {0};

SoftwareShiftRegister::SoftwareShiftRegister() :
   ClkPin(NOPIN), DataPin(NOPIN), OutPutEnablePin(NOPIN), LatchIn(NOPIN), OEHigh(true)
   , LatchInEnableHigh(true), InternalQueueHandler(0) {

}

ErrorType SoftwareShiftRegister::init(gpio_num_t clk, gpio_num_t data, gpio_num_t oe, bool oeHigh
      , gpio_num_t lin, bool latchInHigh, bool setupGPIO) {

	ErrorType et;
   ClkPin = clk;
   DataPin = data;
   OutPutEnablePin = oe;
   LatchIn = lin;
   OEHigh = oeHigh;
   LatchInEnableHigh = latchInHigh;
	
   InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
	ESP_LOGI(LOGTAG,"Internal QueueHandler = %d",(int32_t)InternalQueueHandler);
	if(InternalQueueHandler==nullptr) {
		ESP_LOGE(LOGTAG,"Failed to create queue");
		return ErrorType(errno);
	}

   if(setupGPIO) {
	   gpio_config_t io_conf;
      gpio_num_t PinArray[] = {clk,data,oe,lin};
      const char *PinNames[] = {"clk","data","outputEN","latchIn"};
      for(int i=0;i<((sizeof(PinArray)/sizeof(PinArray[0])));++i) {
         if(PinArray[i]!=NOPIN) {
            memset(&io_conf,0,sizeof(io_conf));
            ESP_LOGI(LOGTAG,"PinName = %s Pin = %d", PinNames[i], PinArray[i]);
	         uint64_t GPIO_INPUT_IO = (1ULL << PinArray[i]);
	         //ESP_LOG_BUFFER_HEX(LOGTAG,&GPIO_INPUT_IO,sizeof(GPIO_INPUT_IO));
	         io_conf.pin_bit_mask = GPIO_INPUT_IO;
	         //set as input mode
	         io_conf.mode = GPIO_MODE_OUTPUT;
	         io_conf.pull_up_en = GPIO_PULLUP_DISABLE; 
	         io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	         et = gpio_config(&io_conf);
	         if(!et.ok()) {
		         ESP_LOGE(LOGTAG,"%s",et.toString());
	         } else {
	            ESP_LOGI(LOGTAG,"GPIO: %s (%d) has been gpio configured", PinNames[i],PinArray[i]);
            }
         } else {
            ESP_LOGI(LOGTAG,"index: %d: %s has no pin configured",i,PinNames[i]);
         }
      }
   }
   return et;
}

void SoftwareShiftRegister::disableOutput() {
   Msg *msg = new Msg(1,0,Msg::CMD);
   if(errQUEUE_FULL==xQueueSend(InternalQueueHandler,&msg,0)) {
      delete msg;
   }

}

void SoftwareShiftRegister::enableOutput() {
   Msg *msg = new Msg(1,1,Msg::CMD);
   if(errQUEUE_FULL==xQueueSend(InternalQueueHandler,&msg,0)) {
      delete msg;
   }

}

void SoftwareShiftRegister::run(void *data) {
   Msg *msg = nullptr;
   while(1) {
      if(xQueueReceive(InternalQueueHandler, &msg, portMAX_DELAY)) {
         //clock out data
         uint32_t data = msg->getData();
         uint32_t bitsToSend = msg->getBitsToSend();
         if(msg->getType()==Msg::CMD) {
            ESP_LOGI(LOGTAG,"PROCESSING CMD");
            switch(data) {
            case 1:
               ESP_LOGI(LOGTAG,"OutputEnable %d",bitsToSend);
               gpio_set_level(OutPutEnablePin, bitsToSend>0?OEHigh:!OEHigh);
               break;
            default:
               break;
            }
         } else {
            ESP_LOGI(LOGTAG,"PROCESSING DATA REQUEST: Data %u, BitsToSend %u",data,bitsToSend);
            //turn off latching
            gpio_set_level(LatchIn,!LatchInEnableHigh);
            gpio_set_level(ClkPin,0);
            for(int i=bitsToSend-1;i>=0;--i) {
               gpio_set_level(DataPin,(data&(1<<i)));
               gpio_set_level(ClkPin,1);
               uint64_t ms = esp_timer_get_time()+1;
               while(ms>=esp_timer_get_time()) ;
               gpio_set_level(ClkPin,0);
            }
            gpio_set_level(LatchIn,LatchInEnableHigh);
         } 
      }
   }
}


void SoftwareShiftRegister::enqueueData(int32_t d, uint32_t bitsToSend) {
   Msg *msg = new Msg(d,bitsToSend,Msg::DATA);
   if(errQUEUE_FULL==xQueueSend(InternalQueueHandler,&msg,0)) {
      delete msg;
   }
}

SoftwareShiftRegister::~SoftwareShiftRegister() {

}

