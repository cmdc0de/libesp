#pragma once

#include <driver/gpio.h>
#include "../../error_type.h"
#include "../../task.h"
#include "../../freertos.h"

namespace libesp {


class SoftwareShiftRegister : public Task {
protected:
   class Msg {
   public:
      enum TYPE {
         DATA, CMD
      };
   public:
      Msg(int32_t data, uint32_t bitsToSend, TYPE t) : Data(data), BitsToSend(bitsToSend), Type(t) {}
      int32_t getData() {return Data;}
      uint32_t getBitsToSend() {return BitsToSend;}
      TYPE getType() {return Type;}
   private:
      int32_t Data;
      uint32_t BitsToSend;
      TYPE Type;
   };
public:
   static const char *LOGTAG;
	static const int QUEUE_SIZE = 4;
	static const int MSG_SIZE = sizeof(SoftwareShiftRegister::Msg*);
public:
   SoftwareShiftRegister();
   ErrorType init(gpio_num_t clk, gpio_num_t data, gpio_num_t oe, bool oeHigh, gpio_num_t lin
         , bool latchInHigh, bool setupGPIO);

	virtual void run(void *data);
   void enqueueData(int32_t d, uint32_t bitsToSend);
   virtual ~SoftwareShiftRegister();
   void disableOutput();
   void enableOutput();
private:
   gpio_num_t ClkPin;
   gpio_num_t DataPin;
   gpio_num_t OutPutEnablePin;
   gpio_num_t LatchIn;
   uint32_t OEHigh:1;
   uint32_t LatchInEnableHigh:1;
	QueueHandle_t InternalQueueHandler;
};

}
