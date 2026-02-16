#include <esp_log.h>
#include "i2c.hpp"
#include "driver/i2c.h"
#include <cstring>

static const char tag[] = "I2C";
using namespace libesp;

ESP32_I2CDevice::ESP32_I2CDevice(gpio_num_t scl, gpio_num_t sda, i2c_port_t p, uint32_t rxBufSize, uint32_t txBufSize) 
	: SCL(scl), SDA(sda), Port(p), RXBufferSize(rxBufSize), TXBufferSize(txBufSize) {

	
}

void ESP32_I2CDevice::deinit() {
	if(I2C_NUM_MAX!=Port) {
		i2c_driver_delete(Port);
		Port = I2C_NUM_MAX;
	}
}

ESP32_I2CDevice::~ESP32_I2CDevice() {
	deinit();
}

////////////////////////////////////////////////////////////////////////////
//
ESP32_I2CSlaveDevice::ESP32_I2CSlaveDevice(gpio_num_t scl, gpio_num_t sda, uint8_t address, i2c_port_t p, uint32_t rxBufSize, uint32_t txBufSize ) 
	: ESP32_I2CDevice(scl,sda,p,rxBufSize,txBufSize), Address(address) {

}


bool ESP32_I2CSlaveDevice::init(bool enablePullUps) {
	bool bRetVal = false;
	i2c_config_t conf;
	conf.mode = I2C_MODE_SLAVE;
	conf.sda_io_num = SDA;
	conf.scl_io_num = SCL;
	if (enablePullUps) {
		conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
		conf.scl_pullup_en = GPIO_PULLUP_ENABLE;

	} else {
		conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
		conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
	}
	conf.slave.addr_10bit_en = 0;
	conf.slave.slave_addr = Address;

	if(ESP_OK==i2c_param_config(Port,&conf)) {
		if(ESP_OK==i2c_driver_install(Port, conf.mode, RXBufferSize, TXBufferSize, 0)) {
			bRetVal = true;
		}
	}
	return bRetVal;
}


int ESP32_I2CSlaveDevice::write(uint8_t *data, uint16_t size, uint16_t ticksToWait) {
	return i2c_slave_write_buffer(getPort(), data, size, ticksToWait );
}

int ESP32_I2CSlaveDevice::read(uint8_t *data, uint16_t size, uint16_t ticksToWait ) {
	return i2c_slave_read_buffer(getPort(), data, size, ticksToWait);
}

////////////////////////////////////////////////////////////////////////////
ESP32_I2CMaster::ESP32_I2CMaster(gpio_num_t scl, gpio_num_t sda, uint32_t clock, i2c_port_t p, uint32_t rxBufSize, uint32_t txBufSize ) 
	: ESP32_I2CDevice(scl,sda,p,rxBufSize,txBufSize), MasterClock(clock), CmdHandle(0) {

}

ESP32_I2CMaster::~ESP32_I2CMaster() {

}

bool ESP32_I2CMaster::init(bool enablePullUps) {
	bool bRetVal = false;
	i2c_config_t conf;
	memset(&conf,0,sizeof(conf));
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = SDA;
	conf.scl_io_num = SCL;
	if(enablePullUps) {	
		conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
		conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	} else {
		conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
		conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
	}
	conf.master.clk_speed = MasterClock;
	if(ESP_OK==i2c_param_config(Port,&conf)) {
		if(ESP_OK==i2c_driver_install(Port, conf.mode, RXBufferSize, TXBufferSize, 0)) {
			bRetVal = true;
			ESP_LOGI(tag,"i2c driver installed");
		} else {
			ESP_LOGE(tag,"ESP_OK==i2c_driver_install");
		}
	} else {
		ESP_LOGE(tag,"ESP_OK==i2c_param_config");
	}
	return bRetVal;
}

bool ESP32_I2CMaster::start(uint8_t addr, bool forWrite) {
	bool bRetVal = false;
	if((CmdHandle=i2c_cmd_link_create())!=0) {
		if(ESP_OK==i2c_master_start(CmdHandle)) {
			if(forWrite) {
				if(ESP_OK==i2c_master_write_byte(CmdHandle, addr<<1|I2C_MASTER_WRITE, true)) { //check the ack since its a command
					bRetVal = true;
				} else {
					ESP_LOGE(tag,"WRITE:  ESP_OK==i2c_master_write_byte");
				}
			} else {
				if(ESP_OK==i2c_master_write_byte(CmdHandle, addr<<1|I2C_MASTER_READ, true)) { //check the ack since its a command
					bRetVal = true;
				} else {
					ESP_LOGE(tag,"READ:  ESP_OK==i2c_master_write_byte");
				}
			}
		} else {
			ESP_LOGE(tag,"ESP_OK==i2c_master_start");
		}
	} else {
		ESP_LOGE(tag,"CmdHandle=i2c_cmd_link_create");
	}
	return bRetVal;
}

ErrorType ESP32_I2CMaster::probe(uint8_t address) {
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_stop(cmd);

	esp_err_t result = i2c_master_cmd_begin(Port, cmd, 10 / portTICK_PERIOD_MS);
   i2c_cmd_link_delete(cmd);
	return result;
}

void ESP32_I2CMaster::scan() {
	ESP_LOGI(tag, "Scanning I2C bus.");

	ErrorType result;
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("00:   ");
	for (uint8_t address = 1; address < 0x78; address++) {
		result = probe(address);

		if (address % 16 == 0) {
			printf("\n%.2x:", address);
		}
		if (result == ESP_OK) {
			printf(" %.2x", address);
		} else {
			printf(" --");
		}
	}
	printf("\n");
}

bool ESP32_I2CMaster::write(uint8_t *data, uint16_t size, bool ack) {
	return ESP_OK==i2c_master_write(CmdHandle,data,size,ack);
}

bool ESP32_I2CMaster::stop(uint16_t ticksToWait) {
	bool bRetVal = false;
	if(ESP_OK==i2c_master_stop(CmdHandle)) {
	   esp_err_t retVal;
		if((retVal=i2c_master_cmd_begin(Port, CmdHandle, ticksToWait))==ESP_OK) {
			bRetVal = true;
		} else {
			ESP_LOGE(tag,"ESP_OK==i2c_master_cmd_begin: %s\n",esp_err_to_name(retVal));
		}
	} else {
		ESP_LOGE(tag,"ESP_OK==i2c_master_stop");
	}
	i2c_cmd_link_delete(CmdHandle);
	return bRetVal;
}

bool ESP32_I2CMaster::read(uint8_t *data, uint16_t size, ACK_TYPE ackType) {
	return ESP_OK==i2c_master_read(CmdHandle, data, size, (i2c_ack_type_t) ackType);
}

#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */

void ESP32_I2CMaster::doIt() {
	static uint8_t DEFAULT_ADDRESS = 0xC0;
	esp_err_t rc;
	i2c_config_t conf;
 	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = GPIO_NUM_21; //GPIO_NUM_18;
	conf.scl_io_num = GPIO_NUM_19;
	//conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
	//conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000; 
	ESP_LOGI(tag, "Configuring I2C");
	rc = i2c_param_config(I2C_NUM_0, &conf);
	ESP_LOGI(tag, "I2C Param Config: %s", esp_err_to_name(rc));
	rc = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	ESP_LOGI(tag, "I2C Driver Install; %s", esp_err_to_name(rc));
	uint16_t rxlen = 4;
	uint8_t rxdata[4] = { 0 };

	// 0x00 as wake up pulse
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	(void)i2c_master_start(cmd);
	(void)i2c_master_write_byte(cmd, I2C_MASTER_WRITE, ACK_CHECK_DIS);
	(void)i2c_master_stop(cmd);
	rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 20);
	ESP_LOGI(tag, "wake; %s", esp_err_to_name(rc));
	(void)i2c_cmd_link_delete(cmd);
	ESP_LOGI(tag, "00 sent");

	cmd = i2c_cmd_link_create();
	(void)i2c_master_start(cmd);
	(void)i2c_master_write_byte(cmd, DEFAULT_ADDRESS | I2C_MASTER_READ, ACK_CHECK_EN);
	(void)i2c_master_read_byte(cmd, rxdata, (i2c_ack_type_t)ACK_VAL);
	rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10);
	ESP_LOGI(tag, "send1; %s", esp_err_to_name(rc));
	(void)i2c_cmd_link_delete(cmd);

	rxlen = rxdata[0];
	ESP_LOGI(tag, "len; %d", rxlen);
	cmd = i2c_cmd_link_create();
  	if (rxlen > 2) {
		(void)i2c_master_read(cmd, &rxdata[1], (rxlen) - 2, (i2c_ack_type_t)ACK_VAL);
	}
	(void)i2c_master_read_byte(cmd, rxdata + (rxlen) - 1, (i2c_ack_type_t) NACK_VAL);
	(void)i2c_master_stop(cmd);
	rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10);
	ESP_LOGI(tag, "send2; %s", esp_err_to_name(rc));
	(void)i2c_cmd_link_delete(cmd);

	ESP_LOG_BUFFER_HEX(tag,rxdata,4);
	//const uint8_t expected[4] = { 0x04, 0x11, 0x33, 0x43 };
	//if (memcmp(rxdata, expected, 4) == 0) {

	//}
	i2c_driver_delete(I2C_NUM_0);
}

void ESP32_I2CMaster::doLED() {
	static uint8_t DEFAULT_ADDRESS = 0x60;
	esp_err_t rc;
	i2c_config_t conf;
 	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = GPIO_NUM_21; //GPIO_NUM_18;
	conf.scl_io_num = GPIO_NUM_22; //GPIO_NUM_19;
	conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
	conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
	conf.master.clk_speed = 100000; 
	ESP_LOGI(tag, "Configuring I2C");
	rc = i2c_param_config(I2C_NUM_0, &conf);
	ESP_LOGI(tag, "I2C Param Config: %s", esp_err_to_name(rc));
	rc = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	ESP_LOGI(tag, "I2C Driver Install; %s", esp_err_to_name(rc));
	//uint16_t rxlen;
	//uint8_t rxdata[4] = { 0 };

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
/*
	// 0x00 as wake up pulse
	(void)i2c_master_start(cmd);
	(void)i2c_master_write_byte(cmd, I2C_MASTER_WRITE, ACK_CHECK_DIS);
	(void)i2c_master_stop(cmd);
	rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 20);
	ESP_LOGI(tag, "wake; %s", esp_err_to_name(rc));
	(void)i2c_cmd_link_delete(cmd);
	ESP_LOGI(tag, "00 sent");

	rxlen = 4;
*/
	
	cmd = i2c_cmd_link_create();
	(void)i2c_master_start(cmd);
	ESP_LOGI(tag, "cmd start");
	(void)i2c_master_write_byte(cmd, DEFAULT_ADDRESS | I2C_MASTER_WRITE, ACK_CHECK_EN);
	(void)i2c_master_write_byte(cmd, 0x20, ACK_CHECK_EN);
	(void)i2c_master_write_byte(cmd, 0xFF, ACK_CHECK_EN);
	//(void)i2c_master_stop(cmd);
	rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10);
	ESP_LOGI(tag, "cmd sent; %s", esp_err_to_name(rc));
	(void)i2c_cmd_link_delete(cmd);

//	rc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10);

	i2c_driver_delete(I2C_NUM_0);
}

