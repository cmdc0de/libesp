
#include "XPT2046.h"
#include <libesp/system.h>
#include <esp_log.h>
#include <freertos/queue.h>
#include <errno.h>
#include <string.h>

using namespace libesp;

const char *XPT2046::LOGTAG = "XPT2046";
static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[XPT2046::TOUCH_QUEUE_SIZE*XPT2046::TOUCH_MSG_SIZE] = {0};

static StaticQueue_t BroadcastInternalQueue;
static uint8_t BroadcastQueueBuffer[XPT2046::QUEUE_SIZE*XPT2046::MSG_SIZE] = {0};

XPT2046::PenEvent XPT2046::PenDownEvt = {XPT2046::PenEvent::PEN_EVENT_DOWN};
XPT2046::PenEvent XPT2046::PenPwrModeEvnt = {XPT2046::PenEvent::PEN_SET_PWR_MODE};

static void IRAM_ATTR touch_isr_handler(void* arg) {
	XPT2046 *pThis = reinterpret_cast<libesp::XPT2046*>(arg);
	QueueHandle_t internalQueueH = pThis->getInternalQueueHandle();
	//ets_printf("internal queueh: %d\n",(int32_t)internalQueueH);
	XPT2046::PenEvent *pe = new XPT2046::PenEvent(XPT2046::PenEvent::PEN_EVENT_DOWN);
	xQueueSendFromISR(internalQueueH, &pe, NULL);
}

//////////////////////////////////////////////
/////static
ErrorType XPT2046::initTouch(gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk
		, spi_host_device_t spiNum, int channel) {
	static const char *LOGTAG = "initTouch";
	ErrorType et;
	//touch bus config
	spi_bus_config_t buscfg;
   buscfg.miso_io_num=miso;
   buscfg.mosi_io_num=mosi;
   buscfg.sclk_io_num=clk;
   buscfg.quadwp_io_num=-1;
   buscfg.quadhd_io_num=-1;
   buscfg.max_transfer_sz=64;
   buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
   buscfg.intr_flags = 0;

	et = libesp::SPIBus::initializeBus(spiNum,buscfg,channel);
	if(!et.ok()) {
		ESP_LOGE(LOGTAG, "Error initializing SPI Bus: %s", et.toString());
	}
	return et;
}

///////////////////////////////////
// instance members
XPT2046::XPT2046(gpio_num_t interruptPin, bool swapXY)
	: Task("XPT2046"), Notifications(), MyDevice(nullptr), InterruptPin(interruptPin), InternalQueueHandler(nullptr), MyControlByte(), PenX(0), PenY(0), PenZ(0), IsPenDown(false), SwapXY(swapXY), BroadcastQueueHandler(nullptr) {
	MyControlByte.c = 0;
	MyControlByte.StartBit = 1;
	MyControlByte.AcquireBits = 0;
	MyControlByte.ModeBit = 0;
	MyControlByte.SerDFR = 0;
	MyControlByte.PwrMode = 0;
}

ErrorType XPT2046::init(SPIBus *bus, gpio_num_t cs) {
	ESP_LOGI(LOGTAG,"init");
	ErrorType et;
	InternalQueueHandler = xQueueCreateStatic(TOUCH_QUEUE_SIZE,TOUCH_MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
	ESP_LOGI(LOGTAG,"Internal QueueHandler = %d",(int32_t)InternalQueueHandler);
	if(InternalQueueHandler==nullptr) {
		ESP_LOGE(LOGTAG,"Failed to create queue");
		return ErrorType(errno);
	}


	BroadcastQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&BroadcastQueueBuffer[0],&BroadcastInternalQueue);
	if(BroadcastQueueHandler==nullptr) {
		ESP_LOGE(LOGTAG,"Failed to create braodcast queue");
		return ErrorType(errno);
	}

	//touch device config
	spi_device_interface_config_t devcfg;
	devcfg.clock_speed_hz=2.5*1000*1000;         //Clock out at 1 MHz
	devcfg.mode=0;          //SPI mode 0
	devcfg.spics_io_num=cs; //CS pin
	devcfg.queue_size=3; //We want to be able to queue 3 transactions at a time
	devcfg.duty_cycle_pos = 0;
	devcfg.cs_ena_pretrans = 0;
	devcfg.cs_ena_posttrans = 0; 
	devcfg.input_delay_ns = 0;
	devcfg.flags = 0;
	devcfg.pre_cb = nullptr;
	devcfg.post_cb = nullptr;

	MyDevice = bus->createMasterDevice(devcfg);
	return et;
}
	
//set up irq -
// one for pos edge pen down
// one for neg edge pen up
void  XPT2046::onStart() {
	ErrorType et;
	ESP_LOGI(LOGTAG,"installing ISR");
	gpio_config_t io_conf;
	//SET UP TOUCH
	//interrupt of rising edge
	//io_conf.intr_type = GPIO_INTR_POSEDGE;
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	//bit mask of the pins, use GPIO0
	ESP_LOGI(LOGTAG,"InterruptPin = %d", InterruptPin);
	const uint64_t GPIO_INPUT_IO = (1ULL << InterruptPin);
	ESP_LOG_BUFFER_HEX(LOGTAG,&GPIO_INPUT_IO,sizeof(GPIO_INPUT_IO));
	io_conf.pin_bit_mask = GPIO_INPUT_IO;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE; 
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	et = gpio_config(&io_conf);
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"%s",et.toString());
	}
	//gpio_isntall_isr_service must have been already called
	//hook isr handler for specific gpio pin
	if(ESP_OK!=gpio_isr_handler_add( InterruptPin, touch_isr_handler, (void*) this)) {
		ESP_LOGE(LOGTAG,"Failed to install isr handler");
	}
	//
}

//if pen irq fires ShouldProcess will be set
// until pen comes off (another interrupt) we will meausre pen position
void XPT2046::run(void *data) {
	ESP_LOGI(LOGTAG,"XPT2046::run");
	PenEvent *pe = nullptr;
	while (1) {
		if(xQueueReceive(InternalQueueHandler, &pe, portMAX_DELAY)) {
			switch(pe->EvtType) {
			case PenEvent::PEN_EVENT_DOWN:
			{
				//ESP_LOGI(LOGTAG,"PEN EVENT DOWN");
				//ESP_LOGI(LOGTAG,"INTERRUPT PIN LEVEL %d",gpio_get_level(InterruptPin));
			 if(gpio_get_level(InterruptPin)==0) {
				int16_t counter = 0;
				while(gpio_get_level(InterruptPin)==0) {
					IsPenDown = true;
					// use use power mode 01 becasue we are using DFR rater than SER 
					uint8_t bo[] = {0xB1,0xc1,0x0,0x91,0x0,0x91,0x0,0xd1,0x0,0x91,0x0,0xd1,0x0,0x91,0x0,0xD0,0x0,0x0,0x0};

					uint8_t bi[sizeof(bo)] = {0x0};
					MyDevice->sendAndReceive(&bo[0],&bi[0],sizeof(bo));
					//ESP_LOG_BUFFER_HEX(LOGTAG, &bi[0],sizeof(bi));
					//ESP_LOGI(LOGTAG,"DECODING...");
					int32_t z1 = bi[2]<<5 | bi[1]>>3;
					int32_t z2 = bi[4]<<5 | bi[3]>>3;
					int32_t z = z1-z2;
					int32_t tax = bi[6]<<5| bi[5]>>3; //throw away x
					int32_t x1 = bi[8]<<5 | bi[7]>>3;
					int32_t y1 = bi[10]<<5 | bi[9]>>3;
					int32_t x2 = bi[12]<<5| bi[11]>>3;
					int32_t y2 = bi[14]<<5| bi[13]>>3;
					int32_t x3 = bi[16]<<5| bi[15]>>3;
					int32_t y3 = bi[18]<<5| bi[17]>>3;
				
					//ESP_LOGI(LOGTAG,"z1 %d z2 %d, z:%d, x1:%d y1:%d, x2:%d y2:%d, x3:%d y3:%d", z1,z2,z,x1,y1,x2,y2,x3,y3);
					if(SwapXY) {
						PenY = (x2+x1)/2;
						PenX = (y2+y1)/2;
					} else {
						PenX = (x2+x1)/2;
						PenY = (y2+y1)/2;
					}
					PenZ = z;
					ESP_LOGI(LOGTAG,"PenX: %d, PenY: %d, PenZ %d",PenX, PenY, PenZ);
					if(++counter>4) {
						counter = 0;
						TouchNotification *tn = new TouchNotification(PenX,PenY,PenZ,true);
						if(errQUEUE_FULL==xQueueSend(BroadcastQueueHandler,&tn,0)) {
							delete tn;
						}
					} 
					vTaskDelay(80 / portTICK_RATE_MS);
				}
				IsPenDown = false;
				TouchNotification *tn = new TouchNotification(PenX,PenY,PenZ,false);
				if(errQUEUE_FULL==xQueueSend(BroadcastQueueHandler,&tn,0)) {
					delete tn;
				}
			 }
			}
				break;
			case PenEvent::PEN_SET_PWR_MODE: 
			{
				ESP_LOGI(LOGTAG,"PEN EVENT SET PWR MODE");
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
			delete pe;pe = nullptr;
		}
	}
}

void XPT2046::onStop() {
	gpio_isr_handler_remove(InterruptPin);
}

void XPT2046::broadcast() {
	TouchNotification *tn;
	if(xQueueReceive(BroadcastQueueHandler, &tn, 0)) {
		std::set<xQueueHandle>::iterator it = Notifications.begin();
		for(;it!=Notifications.end();++it) {
			ESP_LOGI(LOGTAG,"broadcast");
			TouchNotification *tn1 = 
					  new TouchNotification(tn->getX(),tn->getY(),tn->getZ(),
											tn->isPenDown());
			xQueueHandle handle = (*it);
			if(errQUEUE_FULL==xQueueSend(handle, &tn1, 0)) {
				delete tn1;
			}
		}
		delete tn;
	}
}

/*
* add / remove observers
*/
bool XPT2046::addObserver(const xQueueHandle &o) {
	Notifications.insert(o);
	return true;	
}

bool XPT2046::removeObserver(const xQueueHandle &o) {
	Notifications.erase(o);
	return true;
}

/*
* does not try to acquire just sets pwr mode
* if the power mode turns off the pen the acquision will stay off
*/
void XPT2046::setPwrMode(uint8_t pwrMode) {
	//MyControlByte.PwrMode = pwrMode&PWR_DOWN_MASK;
	//xQueueSend(InternalQueueHandler,(void*)&PenPwrModeEvnt, (TickType_t)0);	
}

XPT2046::~XPT2046() {

}

