
#include "XPT2046.h"
#include <libesp/system.h>
#include <esp_log.h>
#include <freertos/queue.h>
#include <errno.h>

using namespace libesp;

const char *XPT2046::LOGTAG = "XPT2046";
static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[XPT2046::TOUCH_QUEUE_SIZE*XPT2046::TOUCH_MSG_SIZE] = {0};
XPT2046::PenEvent XPT2046::PenDownEvt = {XPT2046::PenEvent::PEN_EVENT_DOWN};
XPT2046::PenEvent XPT2046::PenPwrModeEvnt = {XPT2046::PenEvent::PEN_SET_PWR_MODE};

static void IRAM_ATTR touch_isr_handler(void* arg) {
	XPT2046 *pThis = reinterpret_cast<libesp::XPT2046*>(arg);
	QueueHandle_t internalQueueH = pThis->getInternalQueueHandle();
	xQueueSendFromISR(internalQueueH, &XPT2046::PenDownEvt, NULL);
}
 
XPT2046::XPT2046(SPIDevice *device, uint32_t measurementsToAverage, int32_t msBetweenMeasures, gpio_num_t interruptPin)
	: Task("XPT2046", 4196, 2), Notifications(), MyDevice(device), MeasurementsToAverage(measurementsToAverage), MSBetweenMeasurements(msBetweenMeasures), InterruptPin(interruptPin), InternalQueueHandler(nullptr), MyControlByte() {
	MyControlByte.c = 0;
	MyControlByte.StartBit = 1;
	MyControlByte.AcquireBits = 0;
	MyControlByte.ModeBit = 0;
	MyControlByte.SerDFR = 0;
	MyControlByte.PwrMode = 0;
}

ErrorType XPT2046::init() {
	ErrorType et;
	ESP_LOGI(LOGTAG,"init");
	InternalQueueHandler = xQueueCreateStatic(TOUCH_QUEUE_SIZE,TOUCH_MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
	if(InternalQueueHandler==nullptr) {
		ESP_LOGE(LOGTAG,"Failed to create queue");
		return ErrorType(errno);
	}
	//set up SPI;
	uint8_t retVal;
	if((et=MyDevice->sendAndReceive(MyControlByte.c,retVal)).ok()) {
		ESP_LOGI(LOGTAG,"pwr mode sent to touch");	
	} else {
		ESP_LOGE(LOGTAG,"failed to send pwr mode to touch");
	}
	return et;

}
	
//set up irq -
// one for pos edge pen down
// one for neg edge pen up
void  XPT2046::onStart() {
	ESP_LOGI(LOGTAG,"installing ISR");
	gpio_config_t io_conf;
	//SET UP TOUCH
	//interrupt of rising edge
	io_conf.intr_type = GPIO_INTR_POSEDGE;
	//bit mask of the pins, use GPIO0
	const int32_t GPIO_INPUT_IO = (1ULL << InterruptPin);
	io_conf.pin_bit_mask = GPIO_INPUT_IO;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE; 
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	// END TOUCH SETUP
	//gpio_isntall_isr_service must have been already called
	//hook isr handler for specific gpio pin
	if(ESP_OK!=gpio_isr_handler_add( InterruptPin, touch_isr_handler, (void*) this)) {
		ESP_LOGE(LOGTAG,"Failed to install isr handler");
	}
	//
}

//if pen irq fires ShouldProcess will be set
// until pen comes off (another interrupt) we will meausre pen position
// each position measured will measured, MeasurementsToAverage times, with a 'time between mesaures' as well.
void XPT2046::run(void *data) {
	PenEvent pe;
	while (1) {
		if(xQueueReceive(InternalQueueHandler, &pe, portMAX_DELAY)) {
			switch(pe.EvtType) {
			case PenEvent::PEN_EVENT_DOWN:
			{
				//while pin is low
				uint16_t xPos = 0;
				uint16_t yPos = 0;
				while(gpio_get_level(InterruptPin)==0) {
					uint16_t i=0;
					for(;i<MeasurementsToAverage && gpio_get_level(InterruptPin)==0;i++) {
						uint16_t x,y;
						//do Y first;
						MyControlByte.AcquireBits = 0b001;
						uint8_t buf[3] = {MyControlByte.c,0,0};
						MyDevice->sendAndReceive(&buf[0],sizeof(buf));
						y = (uint16_t(buf[1])<<5)|(uint16_t(buf[2])>>3);
						//xPos
						MyControlByte.AcquireBits = 0b101;
						buf[0] = MyControlByte.c;
						buf[1] = 0;
						buf[2] = 0;
						MyDevice->sendAndReceive(&buf[0],sizeof(buf));
						x = (uint16_t(buf[1])<<5)|(uint16_t(buf[2])>>3);
						xPos+=x;
						yPos+=y;
						ESP_LOGI(LOGTAG,"xPos=%d, yPos=%d, XPOS=%d, YPOS=%d", x, y, xPos, yPos);
						vTaskDelay(MSBetweenMeasurements/portTICK_PERIOD_MS);
					}
					if(i>=MeasurementsToAverage) {
						//send to all observers
					}
				}
			}
				break;
			case PenEvent::PEN_SET_PWR_MODE: 
			{
				uint8_t retVal=0;
				ErrorType et;
				if((et=MyDevice->sendAndReceive(MyControlByte.c,retVal)).ok()) {
					ESP_LOGI(LOGTAG,"pwr mode sent to touch");	
				} else {
					ESP_LOGE(LOGTAG,"failed to send pwr mode to touch");
				}
			}
				break;
			}
		}
	}
}

void XPT2046::onStop() {
	gpio_isr_handler_remove(InterruptPin);
}

/*
* add / remove observers
*/
bool XPT2046::addObserver(const xQueueHandle &o) {
	return true;	
}

bool XPT2046::removeObserver(const xQueueHandle &o) {
	return true;
}

/*
* does not try to acquire just sets pwr mode
* if the power mode turns off the pen the acquision will stay off
*/
void XPT2046::setPwrMode(uint8_t pwrMode) {
	MyControlByte.PwrMode = pwrMode&PWR_DOWN_MASK;
	xQueueSend(InternalQueueHandler,(void*)&PenPwrModeEvnt, (TickType_t)0);	
}

XPT2046::~XPT2046() {

}

