
#include "GC9A01.h"
#include "device/display/basic_back_buffer.h"
#include "device/display/color.h"
#include "device/display/display_types.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "spidevice.h"
#include "error_type.h"
#include "hal/gpio_types.h"
#include "spibus.h"
#include <cstdint>
#include "../../memory/fixed_class_allocator.h"

using namespace libesp;

const char *GC9A01::LOGTAG = "GC9A01";

static void delay_ms (uint32_t Delay_ms) {
	vTaskDelay(Delay_ms/portTICK_PERIOD_MS);
}

struct spi_data {
public:
   spi_data() : dcPin(GPIO_NUM_NC), dcLevel(0) {}
   spi_data(gpio_num_t p, int32_t l) : dcPin(p), dcLevel(l) {}
   gpio_num_t dcPin;
   int32_t dcLevel;
public:
   //static void *operator new(size_t size) {
   //   return Allocator.allocate();
   //}
   //static void operator delete(void *p) {
   //   Allocator.free((spi_data *)p);
   //}
private:
   spi_data(const spi_data &cpy);
   spi_data &operator=(const spi_data &rhs);
   //static FixedClassAllocator<spi_data, 5> Allocator;
};

//FixedClassAllocator<spi_data, 5> spi_data::Allocator;
static gpio_num_t DATA_CMD_PIN = GPIO_NUM_5;

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
static IRAM_ATTR void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
   //spi_data *data = (spi_data *)t->user;
   ///gpio_set_level(data->dcPin, data->dcLevel);
   //delete data;
	int dc=reinterpret_cast<int>(t->user);
   gpio_set_level(DATA_CMD_PIN, dc);
   t->user = nullptr;
}

//initialize SPI bus
ErrorType GC9A01::spiInit(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk, int channel
      ,spi_host_device_t spiNum) {
   ErrorType et;
   
   spi_bus_config_t buscfg;
   memset(&buscfg, 0, sizeof(buscfg));
   buscfg.mosi_io_num=mosi;
   buscfg.miso_io_num=miso;
   buscfg.sclk_io_num=sclk;
   buscfg.quadwp_io_num=-1;
   buscfg.quadhd_io_num=-1;
   buscfg.max_transfer_sz=SPI_MAX_DMA_LEN;
   buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
   buscfg.intr_flags = 0;
    
   et = libesp::SPIBus::initializeBus(spiNum,buscfg,channel);
   if(!et.ok()) {
      ESP_LOGE(LOGTAG,"error initing BUS for touch %s", et.toString());
   } else {
      ESP_LOGI(LOGTAG,"SPIBus initiatlized for display");
   }
   return et;
}

GC9A01::GC9A01(DISPLAY_ROTATION r)
      : SPI(0), PinDC(GPIO_NUM_NC), PinRST(GPIO_NUM_NC), DisplayWidth(240), DisplayHeight(240)
        , Rotation(r), SpiSemaphoreHandle(0), Flags(), Ledc_cConfig(), Ledc_tConfig() {

      memset(&Ledc_cConfig,0,sizeof(Ledc_cConfig));
      memset(&Ledc_tConfig,0,sizeof(Ledc_tConfig));
      Flags.Flags = 0;
}

GC9A01::~GC9A01() {

}

/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

static const lcd_init_cmd_t lcd_init_cmds[]={
    {0xef,{0},0},
    {0xeb,{0x14},1},
    {0xfe,{0},0},
    {0xef,{0},0},
    {0xeb,{0x14},1},
    {0x84,{0x40},1},
    {0x85,{0xff},1},
    {0x86,{0xff},1},
    {0x87,{0xff},1},
    {0x88,{0x0a},1},
    {0x89,{0x21},1},
    {0x8a,{0x00},1},
    {0x8b,{0x80},1},
    {0x8c,{0x01},1},
    {0x8d,{0x01},1},
    {0x8e,{0xff},1},
    {0x8f,{0xff},1},
    {GC9A01::Cmd_DisplayFunctionControl,{0x00,0x20},2},// Scan direction S360 -> S1
    //{Cmd_MADCTL,{0x08},1},//MemAccessModeSet(0, 0, 0, 1);
    //{Cmd_COLMOD,{ColorMode_MCU_16bit&0x77},1},
    {0x90,{0x08,0x08,0x08,0x08},4},
    {0xbd,{0x06},1},
    {0xbc,{0x00},1},
    {0xff,{0x60,0x01,0x04},3},
    {GC9A01::Cmd_PWCTR2,{0x13},1},
    {GC9A01::Cmd_PWCTR3,{0x13},1},
    {GC9A01::Cmd_PWCTR4,{0x22},1},
    {0xbe,{0x11},1},
    {0xe1,{0x10,0x0e},2},
    {0xdf,{0x21,0x0c,0x02},3},
    {GC9A01::Cmd_GAMMA1,{0x45,0x09,0x08,0x08,0x26,0x2a},6},
    {GC9A01::Cmd_GAMMA2,{0x43,0x70,0x72,0x36,0x37,0x6f},6},
    {GC9A01::Cmd_GAMMA3,{0x45,0x09,0x08,0x08,0x26,0x2a},6},
    {GC9A01::Cmd_GAMMA4,{0x43,0x70,0x72,0x36,0x37,0x6f},6},
    {0xed,{0x1b,0x0b},2},
    {0xae,{0x77},1},
    {0xcd,{0x63},1},
    {0x70,{0x07,0x07,0x04,0x0e,0x0f,0x09,0x07,0x08,0x03},9},
    {GC9A01::Cmd_FRAMERATE,{0x34},1},// 4 dot inversion
    {0x62,{0x18,0x0D,0x71,0xED,0x70,0x70,0x18,0x0F,0x71,0xEF,0x70,0x70},12},
    {0x63,{0x18,0x11,0x71,0xF1,0x70,0x70,0x18,0x13,0x71,0xF3,0x70,0x70},12},
    {0x64,{0x28,0x29,0xF1,0x01,0xF1,0x00,0x07},7},
    {0x66,{0x3C,0x00,0xCD,0x67,0x45,0x45,0x10,0x00,0x00,0x00},10},
    {0x67,{0x00,0x3C,0x00,0x00,0x00,0x01,0x54,0x10,0x32,0x98},10},
    {0x74,{0x10,0x85,0x80,0x00,0x00,0x4E,0x00},7},
    {0x98,{0x3e,0x07},2},
    {GC9A01::Cmd_TEON,{0},0},// Tearing effect line on
    {0, {0}, 0xff},//END
};

ErrorType GC9A01::init(SPIBus *bus, gpio_num_t cs, gpio_num_t dataCmdPin, gpio_num_t resetPin
      , gpio_num_t backlightPin, BasicBackBuffer * bb, SemaphoreHandle_t handle) {
   ErrorType et;
   SpiSemaphoreHandle = handle;
   PinDC = dataCmdPin;
   PinRST = resetPin;
   PinBL = backlightPin;

   gpio_config_t conf;
   conf.intr_type = GPIO_INTR_DISABLE;
   conf.mode = GPIO_MODE_OUTPUT;
   conf.pin_bit_mask = (1ULL<<PinDC);
   conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
   conf.pull_up_en = GPIO_PULLUP_DISABLE;
   et = gpio_config(&conf);
   if(et.ok()) {
      if(PinRST!=GPIO_NUM_NC) {
         conf.pin_bit_mask = (1ULL<<PinRST);
         et = gpio_config(&conf);
         gpio_set_level(PinRST, 1);
      }
      if(et.ok() && PinBL!=GPIO_NUM_NC) {
         conf.pin_bit_mask = (1ULL<<PinBL);
         et = gpio_config(&conf);
         gpio_set_level(PinBL, 0);
      }
      if(et.ok()) {
         spi_device_interface_config_t devcfg;
         memset(&devcfg, 0, sizeof(devcfg));
         devcfg.clock_speed_hz = 20*1000*1000;
         devcfg.mode = 0;
         devcfg.spics_io_num = cs;
         devcfg.queue_size = 5;
         devcfg.pre_cb = lcd_spi_pre_transfer_callback;
	      SPI = bus->createMasterDevice(devcfg, SpiSemaphoreHandle);
	      if(!SPI) {
		      ESP_LOGE(LOGTAG,"failed createInitDevice");
		      //TODO FIXME
		      return ErrorType();
	      }
         reset();
         int32_t cmd = 0;
         while (lcd_init_cmds[cmd].databytes!=0xff) {
            writeCmd(lcd_init_cmds[cmd].cmd);
            writeNData(lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes&0x1F);
            if (lcd_init_cmds[cmd].databytes&0x80) {
               delay_ms(100);
            }
            cmd++;
         }
         memAccessModeSet(PORTAIT_TOP_LEFT,0,0,1);
         setPixelFormat(bb);

         inversionMode(true);
         sleepMode(false);
	      delay_ms(120);
         powerDisplay(true);
         delay_ms(20);
         ESP_LOGI(LOGTAG,"init gc9a01 before fill screen");
         bb->fillScreen(RGBColor::BLACK);
         ESP_LOGI(LOGTAG,"init gc9a01 after fill screen before swap");
         swap(bb);
         ESP_LOGI(LOGTAG,"init gc9a01 after swap");
         backlight(100);
      } else {
         ESP_LOGE(LOGTAG, "failed gpio init: %s", et.toString());
      }
   } else {
      ESP_LOGE(LOGTAG, "failed gpio init: %s", et.toString());
   }
   return et;
}

ErrorType GC9A01::initBackLightPWM(ledc_timer_t led_timer, ledc_channel_t channel) {
   ErrorType et;
   Flags.BackLightPWM = 1;
	Ledc_tConfig.speed_mode=LEDC_LOW_SPEED_MODE ;
	Ledc_tConfig.duty_resolution=LEDC_TIMER_8_BIT;
	Ledc_tConfig.timer_num=led_timer;
	Ledc_tConfig.freq_hz=1000;
	Ledc_tConfig.clk_cfg=LEDC_AUTO_CLK;
	et = ledc_timer_config(&Ledc_tConfig);

   if(et.ok()) {
      Ledc_cConfig.gpio_num=PinBL;
	   Ledc_cConfig.speed_mode=LEDC_LOW_SPEED_MODE;
	   Ledc_cConfig.channel=channel;
	   Ledc_cConfig.intr_type=LEDC_INTR_DISABLE;
	   Ledc_cConfig.timer_sel=led_timer;
	   Ledc_cConfig.duty=0;
	   Ledc_cConfig.hpoint=0;
	   et = ledc_channel_config(&Ledc_cConfig);
   }
   return et;
}

bool GC9A01::setRotation(DISPLAY_ROTATION rotation) {
   return memAccessModeSet(rotation, Flags.VerticalMirror, Flags.HorizontalMirror, Flags.IsBGR);
}

ErrorType GC9A01::backlight(uint16_t level) {
   ErrorType et;
   if(supportBackLightPWM()) {
      //set up PWM
	   uint16_t MaxD = (1<<(int)Ledc_tConfig.duty_resolution)-1;
	   if(level>=100) {
         level=MaxD;
      } else {
		   level=level*(MaxD/(float)100);
	   }
	   Ledc_cConfig.duty=level;
	   ledc_channel_config(&Ledc_cConfig);
   } else {
      if(PinBL!=GPIO_NUM_NC) {
         et = gpio_set_level(PinBL, (level>0?1:0));
      }
   }
   return et;
}

bool GC9A01::setPixelFormat(const BasicBackBuffer *backBuff) {
   LIB_PIXEL_FORMAT pf = backBuff->getPixelFormat();
   uint8_t colorMode = 0;
   switch(pf) {
      case LIB_PIXEL_FORMAT::FORMAT_12_BIT:
         colorMode = ColorMode_MCU_12bit;
         break;
      case LIB_PIXEL_FORMAT::FORMAT_16_BIT:
         //if(Flags.IsBGR) {
            colorMode = ColorMode_MCU_16bit;
         //} else {
          //  colorMode = ColorMode_RGB_16bit;
         //}
         break;
      case LIB_PIXEL_FORMAT::FORMAT_18_BIT:
         //if(Flags.IsBGR) {
            colorMode = ColorMode_MCU_18bit;
         //} else {
          //  colorMode = ColorMode_RGB_18bit;
         //}
         break;
   }
	writeCmd(Cmd_COLMOD);
   uint8_t d = colorMode & 0x77;
	return writeNData(&d, 1);
}

bool GC9A01::memAccessModeSet(DISPLAY_ROTATION Rotation, bool VertMirror, bool HorizMirror, bool IsBGR) {
	uint8_t Ret=0;
   Flags.IsBGR = IsBGR;
   Flags.HorizontalMirror = HorizMirror;
   Flags.VerticalMirror = VertMirror;

   bool retVal = writeCmd(Cmd_MADCTL);

   if(retVal) {
	   switch (Rotation) {
   	case LANDSCAPE_TOP_RIGHT:
		   Ret = 0;
		   break;
	   case LANDSCAPE_TOP_LEFT:
		   Ret = MADCTL_MX;
		   break;
	   case LANDSCAPE_BOTTOM_RIGHT:
		   Ret = MADCTL_MY;
		   break;
	   case LANDSCAPE_BOTTOM_LEFT:
		   Ret = MADCTL_MX | MADCTL_MY;
		   break;
	   case PORTAIT_BOTTOM_LEFT:
		   Ret = MADCTL_MV;
		   break;
      case PORTAIT_TOP_RIGHT: //5:
		   Ret = MADCTL_MV | MADCTL_MX;
		   break;
      case PORTAIT_TOP_LEFT: //6:
		   Ret = MADCTL_MV | MADCTL_MY;
		   break;
	   case PORTAIT_BOTTOM_RIGHT:
		   Ret = MADCTL_MV | MADCTL_MX | MADCTL_MY;
		   break;
	}
	if (Flags.VerticalMirror)
		Ret = MADCTL_ML;
	if (Flags.HorizontalMirror)
		Ret = MADCTL_MH;

	if (Flags.IsBGR)
		Ret |= MADCTL_BGR;
	   retVal = writeNData(&Ret, 1);
   }
   return retVal;
}

void GC9A01::reset() {
	gpio_set_level(PinRST,0);
	delay_ms(10);
	gpio_set_level(PinRST,1);
	delay_ms(150);
}

void GC9A01::sleepMode(bool sleep) {
	if (sleep)
		writeCmd(Cmd_SLPIN);
	else
		writeCmd(Cmd_SLPOUT);

	delay_ms(500);
}

void GC9A01::inversionMode(bool invert) {
	if (invert)
		writeCmd(Cmd_INVON);
	else
		writeCmd(Cmd_INVOFF);
}

void GC9A01::powerDisplay(bool On) {
	if (On)
		writeCmd(Cmd_DISPON);
	else
		writeCmd(Cmd_DISPOFF);
}

void GC9A01::columnSet(uint16_t ColumnStart, uint16_t ColumnEnd) {
	if (ColumnStart > ColumnEnd)
		return;
	if (ColumnEnd > getWidth())
		return;

	writeCmd(Cmd_CASET);
   uint8_t data = ColumnStart >> 8;
   writeNData(&data,1);
   data = ColumnStart & 0xFF;
   writeNData(&data,1);
   data = ColumnEnd >> 8;
   writeNData(&data,1);
   data = ColumnEnd & 0xFF;
   writeNData(&data,1);
}

void GC9A01::rowSet(uint16_t RowStart, uint16_t RowEnd) {
	if (RowStart > RowEnd)
		return;
	if (RowEnd > getHeight())
		return;

	writeCmd(Cmd_RASET);
   uint8_t data = RowStart >> 8;
   writeNData(&data,1);
   data = RowStart & 0xFF;
   writeNData(&data,1);
   data = RowEnd >> 8;
   writeNData(&data, 1);
   data = RowEnd & 0xFF;
   writeNData(&data, 1);
}

ErrorType GC9A01::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	columnSet(x0, x1);
	rowSet(y0, y1);

	writeCmd(Cmd_RAMWR);
   return ErrorType();
}

ErrorType GC9A01::swap(BasicBackBuffer *backBuffer) {
   ErrorType et;
   const uint8_t *p = backBuffer->getBackBuffer();
   //ESP_LOGI(LOGTAG,"swap %p",p);
   //ESP_LOG_BUFFER_HEX(LOGTAG, p,6);
   int32_t len = backBuffer->getBackBufferSize();
   //ESP_LOGI(LOGTAG,"swap bufer length = %d",len);
   setWindow(0, 0, getWidth()-1, getHeight()-1);
   if(getWidth()==backBuffer->getBufferWidth() && getHeight()==backBuffer->getBufferHeight()) {
      //SPI->setDebug(true);
      et = writeNData(p,len);
      //SPI->setDebug(false);
   } else {
      et = writeNData(p,len);
   }
   return et;
}

bool GC9A01::writeCmd(uint8_t c) {
   return writeN(0,&c,1);
}

bool GC9A01::writeNData(const uint8_t *data, int nbytes) {
   return writeN(1,data,nbytes);
}

bool GC9A01::write16Data(const uint16_t &data) {
	uint8_t buf[2];
	buf[0] = data >> 8;
	buf[1] = data & 0xFF;
	return writeN(1, &buf[0], sizeof(buf));
}

bool GC9A01::writeN(char dc, const uint8_t *data, int nbytes) {
	void *ud = reinterpret_cast<void*>(dc);
	ErrorType et = SPI->send(data,nbytes,ud);
	return et.ok();
}

