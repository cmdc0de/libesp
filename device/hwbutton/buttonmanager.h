#pragma once

#include "../../error_type.h"
#include "../../observer_base.h"
#include "../../system.h"
#include "device/display/layout.h"
#include <driver/gpio.h>
#include "../../freertos.h"


namespace libesp {

template<uint16_t MaxEvents=10, uint16_t MaxListeners=5, uint16_t TotalButtons=6, uint16_t MAX_OBSERVERS=2>
class ButtonManager : public ObserverBase<MAX_OBSERVERS> {
public:
   static constexpr const char *LOGTAG = "ButtonManager";
public:
	struct ButtonInfo {
      ButtonInfo() : gpio(NOPIN), DownIsLow(false), BM(0) {}
		ButtonInfo(gpio_num_t b, bool downIsLow) : gpio(b), DownIsLow(downIsLow), BM(0) {}
      gpio_num_t gpio;
		bool DownIsLow;
      ButtonManager *BM; //used internally
	};
	//event
	class ButtonEvent {
   public:
      ButtonEvent(): Button(NOPIN), Index(0), OnDown(0) {}
		ButtonEvent(const ButtonInfo *d, uint16_t index, bool bd) : ButtonData(d), Index(index), OnDown(bd) {}
		gpio_num_t getButton() const {return ButtonData[Index].gpio;}
		bool wasReleased() const { return OnDown==0;}
   private:
      const ButtonInfo *ButtonData;
		uint16_t Index;
		bool OnDown;
	};
public:
   void resolveAndEmit() {
		for(uint16_t i=0;i<TotalButtons;++i) {
		   //something changed
         if((CurrentIndexMap&(1<<i))!=(LastIndexMap&(1<<i))) {
            ESP_LOGI(LOGTAG, "GPIO Pin: %d ", ButtonData[i].gpio);
            if(ButtonData[i].DownIsLow) {
					 if((CurrentIndexMap&(1<<i))==0) {
						 this->broadcast(new ButtonEvent(ButtonData,i,true));
					 } else {
						 this->broadcast(new ButtonEvent(ButtonData,i,false));
					 }
				} else {
					 if((CurrentIndexMap&(1<<i))) {
						 this->broadcast(new ButtonEvent(ButtonData,i,true));
					 } else {
						 this->broadcast(new ButtonEvent(ButtonData,i,false));
					 }
				}
			}
		}
		//set up last index
		LastIndexMap = CurrentIndexMap;
	}
	//if setPoll is false then we'll use interrupts otherwise user must call poll() to check gpios
   ButtonManager(bool bPoll) : ButtonData(0), IsPoll(bPoll), LastIndexMap(0), CurrentIndexMap(0) {}
   static void IRAM_ATTR button_isr_handler(void* arg) {
      ButtonInfo *bd = (ButtonInfo*) arg;
      bd->BM->levelChanged(bd);
   }
   void levelChanged(ButtonInfo *bd) {
      uint32_t level = gpio_get_level(bd->gpio);
      //calc index
      uint32_t index = ButtonData-bd;
      if(1==level) {
         CurrentIndexMap |= (1<<index);
      } else {
         CurrentIndexMap &= ~(1<<index);
      }
   }
	ErrorType init(ButtonInfo *buttonInfo, bool setUpGPIO) {
	   ErrorType et;
      ButtonData = buttonInfo;
      for(int i=0;i<TotalButtons;++i) {
         ButtonData[i].BM = this;
      }
      if(!setUpGPIO) {
         return et;
      }
	   gpio_config_t io_conf;
      for(int i=0;i<TotalButtons;++i) {
         memset(&io_conf,0,sizeof(io_conf));
         if(ButtonData[i].DownIsLow) {
	         io_conf.intr_type = GPIO_INTR_NEGEDGE;
         } else {
	         //interrupt of rising edge
	         io_conf.intr_type = GPIO_INTR_POSEDGE;
         }
	      //bit mask of the pins, use GPIO0
	      ESP_LOGI(LOGTAG,"Button Pin = %d", ButtonData[i].gpio);
	      const uint64_t GPIO_INPUT_IO = (1ULL << ButtonData[i].gpio);
	      ESP_LOG_BUFFER_HEX(LOGTAG,&GPIO_INPUT_IO,sizeof(GPIO_INPUT_IO));
	      io_conf.pin_bit_mask = GPIO_INPUT_IO;
	      //set as input mode
	      io_conf.mode = GPIO_MODE_INPUT;
	      io_conf.pull_up_en = GPIO_PULLUP_DISABLE; 
	      io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	      et = gpio_config(&io_conf);
	      if(!et.ok()) {
		      ESP_LOGE(LOGTAG,"%s",et.toString());
	      } else {
	         ESP_LOGI(LOGTAG,"GPIO: %d has been gpio configured", ButtonData[i].gpio);
         }
	      //gpio_isntall_isr_service must have been already called
	      //hook isr handler for specific gpio pin
         if(!IsPoll) {
	         ESP_LOGI(LOGTAG,"installing ISRs");
	         if(ESP_OK!=gpio_isr_handler_add( ButtonData[i].gpio, button_isr_handler, &ButtonData[i])) {
		         ESP_LOGE(LOGTAG,"Failed to install isr handler");
	         }
         }
      }
      return et;
   }
	ErrorType poll() {
      ErrorType et;
      if(IsPoll) {
   		resetCurrentIndexes();
   		for(uint16_t i=0;i<TotalButtons;++i) {
            if(gpio_get_level(ButtonData[i].gpio)) {
               CurrentIndexMap |= (1<<i);
            } else {
               CurrentIndexMap = CurrentIndexMap&(~(1<<i));
            }
         }
      } else {
         et = ErrorType::INVALID_PARAM;
      }
      return et;
	}
protected:
	void resetCurrentIndexes() {
      CurrentIndexMap = 0;
		for(uint16_t i=0;i<TotalButtons;++i) {
         if(ButtonData[i].DownIsLow) {
            CurrentIndexMap |= 1<<i;
			}
		} 
	}
   void onBroadcast() {
      resolveAndEmit();
		resetCurrentIndexes();
   }
private:
   ButtonInfo *ButtonData;
	uint32_t IsPoll:1;
	uint32_t LastIndexMap:14;
	uint32_t CurrentIndexMap:14;
};

}

