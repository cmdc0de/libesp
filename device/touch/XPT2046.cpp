
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
XPT2046::PenEvent XPT2046::PenDownEvt = {XPT2046::PenEvent::PEN_EVENT_DOWN};
XPT2046::PenEvent XPT2046::PenPwrModeEvnt = {XPT2046::PenEvent::PEN_SET_PWR_MODE};

static void IRAM_ATTR touch_isr_handler(void* arg) {
	XPT2046 *pThis = reinterpret_cast<libesp::XPT2046*>(arg);
	QueueHandle_t internalQueueH = pThis->getInternalQueueHandle();
	//ets_printf("internal queueh: %d\n",(int32_t)internalQueueH);
	XPT2046::PenEvent *pe = new XPT2046::PenEvent(XPT2046::PenEvent::PEN_EVENT_DOWN);
	xQueueSendFromISR(internalQueueH, &pe, NULL);
}
 
XPT2046::XPT2046(uint32_t measurementsToAverage, int32_t msBetweenMeasures, gpio_num_t interruptPin)
	: Task("XPT2046"), Notifications(), MyDevice(nullptr), MeasurementsToAverage(measurementsToAverage), MSBetweenMeasurements(msBetweenMeasures), InterruptPin(interruptPin), InternalQueueHandler(nullptr), MyControlByte() {
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

#if 0
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	uint64_t mask = (1ULL<<cs);
	io_conf.pin_bit_mask = mask;
	io_conf.pull_down_en =GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	if(ESP_OK==gpio_config(&io_conf)) {
		ESP_LOGI(LOGTAG,"CS PIN CONFIG OK");
	} else {
		ESP_LOGE(LOGTAG,"CS PIN CONFIG NOT OK");
	}

	spi_device_handle_t spi = MyDevice->getDeviceHandle();
	ESP_LOGI(LOGTAG,"ret: %d", (int)spi);
	uint8_t v = 0x90;
	ESP_LOGI(LOGTAG,"init send and receive");
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));       //Zero out the transaction
	t.length=8;                 //Len is in bytes, transaction length is in bits.
	t.tx_data[0]=v;               //Data
	t.flags = SPI_TRANS_USE_TXDATA|SPI_TRANS_USE_RXDATA;
	t.user=(void*)1;                //D/C needs to be set to 1
	ret=spi_device_polling_transmit(spi, &t);  //Transmit!
	ESP_LOGI(LOGTAG,"ret: %d", (int)t.rx_data[0]);
	assert(ret==ESP_OK); 
	//TouchDev->sendAndReceive(v,v);
//	uint8_t retVal;
//	if((et=MyDevice->sendAndReceive(MyControlByte.c,retVal)).ok()) {
//		ESP_LOGI(LOGTAG,"pwr mode sent to touch");	
//	} else {
//		ESP_LOGE(LOGTAG,"failed to send pwr mode to touch");
//	}
#endif
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
	ESP_LOGI(LOGTAG,"end start: loop times: %d", MeasurementsToAverage);
}

//if pen irq fires ShouldProcess will be set
// until pen comes off (another interrupt) we will meausre pen position
// each position measured will measured, MeasurementsToAverage times, with a 'time between mesaures' as well.
void XPT2046::run(void *data) {
	ESP_LOGI(LOGTAG,"XPT2046::run");
	PenEvent *pe = nullptr;
	while (1) {
		if(xQueueReceive(InternalQueueHandler, &pe, portMAX_DELAY)) {
#if 0
			if(ESP_OK!=gpio_set_level(GPIO_NUM_27,0)) {
				ESP_LOGE(LOGTAG,"problem setting GPIO level");
			}
			if(0!=gpio_get_level(GPIO_NUM_27)) {
				ESP_LOGE(LOGTAG,"level not set GPIO level");
			}
#endif
			switch(pe->EvtType) {
			case PenEvent::PEN_EVENT_DOWN:
			{
				ESP_LOGI(LOGTAG,"PEN EVENT DOWN");
				uint16_t xPos = 0;
				uint16_t yPos = 0;
				ESP_LOGI(LOGTAG,"INTERRUPT PIN LEVEL %d",gpio_get_level(InterruptPin));
				//dummy measure
				while(gpio_get_level(InterruptPin)==0) {
#if 1
				// use use power mode 01 becasue we are using DFR rater than SER 
				uint8_t bo[] = {0xB1,0xc1,0x0,0x91,0x0,0x91,0x0,0xd1,0x0,0x91,0x0,0xd1,0x0,0x91,0x0,0xD0,0x0,0x0,0x0};
#else
				uint8_t bo[] = {0xB1,0xc1,0x0,0x91,0x0,0x91,0x0,0xd1,0x0,0x91,0x0,0xd1,0x0,0x91,0x0,0xD0,0x0,0x0,0x0};
#endif
				uint8_t bi[sizeof(bo)] = {0x0};
				MyDevice->sendAndReceive(&bo[0],&bi[0],sizeof(bo));
				ESP_LOG_BUFFER_HEX(LOGTAG, &bi[0],sizeof(bi));
				ESP_LOGI(LOGTAG,"DECODING...");
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
				
				ESP_LOGI(LOGTAG,"z1 %d z2 %d, z:%d, x1:%d y1:%d, x2:%d y2:%d, x3:%d y3:%d",
									 z1,z2,z,x1,y1,x2,y2,x3,y3);
				//	uint8_t buf[2] = {0x90,0};
				//	MyDevice->sendAndReceive(buf[0],buf[0]);
				//	MyDevice->sendAndReceive(buf[1],buf[1]);
#if 0
					int i=0;
					for(;i<MeasurementsToAverage && gpio_get_level(InterruptPin)==0;i++) {
						ESP_LOGI(LOGTAG,"measurement %d of %d",i, MeasurementsToAverage);
						//do Y first;
						//MyControlByte.AcquireBits = 0b001;
						//ESP_LOG_BUFFER_HEX(LOGTAG,&MyControlByte.c,1);
						ESP_LOGI(LOGTAG,"SEND AND RECEIVE Y");
						buf[0] = 0x90;
						buf[1] = 0x00;
						MyDevice->sendAndReceive(buf[0],buf[0]);
						MyDevice->sendAndReceive(buf[1],buf[1]);
						ESP_LOGI(LOGTAG,"1: %d, 2: %d",(int)buf[0], (int)buf[1]);
						int y = (uint16_t(buf[0])<<5)|(uint16_t(buf[1])>>3);
						//xPos
						//MyControlByte.AcquireBits = 0b101;
						buf[0] = 0xD0;
						buf[1] = 0;
						ESP_LOGI(LOGTAG,"SENDAND RECEIVE X");
						MyDevice->sendAndReceive(buf[0],buf[0]);
						MyDevice->sendAndReceive(buf[1],buf[1]);
						ESP_LOGI(LOGTAG,"1: %d, 2: %d",(int)buf[0], (int)buf[1]);
						int x = (uint16_t(buf[0])<<5)|(uint16_t(buf[1])>>3);
						xPos+=x;
						yPos+=y;
						ESP_LOGI(LOGTAG,"xPos=%d, yPos=%d, XPOS=%d, YPOS=%d", x, y, xPos, yPos);
						vTaskDelay(MSBetweenMeasurements/portTICK_PERIOD_MS);
					}
					if(i>=MeasurementsToAverage) {
						//notify
					}
#endif
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
#if 0
			gpio_set_level(GPIO_NUM_27,1);
#endif
			delete pe;pe = nullptr;
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
	//MyControlByte.PwrMode = pwrMode&PWR_DOWN_MASK;
	//xQueueSend(InternalQueueHandler,(void*)&PenPwrModeEvnt, (TickType_t)0);	
}

XPT2046::~XPT2046() {

}

