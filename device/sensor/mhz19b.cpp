#include <string.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "mhz19b.h"

static const char *TAG = "mhz19b";
using namespace libesp;

#define CHECK(x) do { ErrorType __; if (!(__ = x).ok()) return __ ; } while (0)

MHZ19::MHZ19(): UartPort(UART_NUM_MAX), ReadBuffer(), LastCO2Value(0), LastTimestamp(0), Version(), Range(0xFFFF) {

}


ErrorType MHZ19::init(uart_port_t uart_port, gpio_num_t tx_gpio, gpio_num_t rx_gpio) {
  // Clear version
  memset(Version, 0, sizeof(Version));
  UartPort = uart_port;

  uart_config_t uart_config;
  memset(&uart_config,0,sizeof(uart_config));
  uart_config.baud_rate = 9600;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity    = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_APB;
  uart_config.rx_flow_ctrl_thresh = 127;

  CHECK(uart_driver_install(UartPort, sizeof(ReadBuffer) * 2, 0, 0, NULL, 0));
  CHECK(uart_param_config(UartPort, &uart_config));
  CHECK(uart_set_pin(UartPort, tx_gpio, rx_gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  LastTimestamp = esp_timer_get_time();
  return ESP_OK;
}

ErrorType MHZ19::shutdown() {
    return ESP_OK;
}

bool MHZ19::detect() {
    uint16_t range;
    // Check valid PPM range
    if ((getRange(range) == ESP_OK) && (range > 0))
        return true;

    // Sensor not detected, or invalid range returned
    // Try recover by calling setRange(MHZ19_RANGE_5000);
    return false;
}

bool MHZ19::isWarmingUp(bool smartWarmUp) {
    // Wait at least 3 minutes after power-on
    if (esp_timer_get_time() < MHZ19_WARMING_UP_TIME_US) {
        if (smartWarmUp) {
            ESP_LOGI(TAG, "Using smart warming up detection ");

            int16_t co2, last_co2;
            last_co2 = LastCO2Value;
            // Sensor returns valid data after CPU reset and keep sensor powered
            if (!readCO2(co2).ok())
                return false;
            if ((last_co2 != 0) && (last_co2 != co2))
                // CO2 value changed since last read, no longer warming-up
                return false;
        }
        // Warming-up
        return true;
    }
    // Not warming-up
    return false;
}

bool MHZ19::isReady() {
    // Minimum CO2 read interval (Built-in LED flashes)
    if ((esp_timer_get_time() - LastTimestamp) > MHZ19_READ_INTERVAL_MS) {
        return true;
    }
    return false;
}

ErrorType MHZ19::readCO2(int16_t &co2) {
    // Send command "Read CO2 concentration"
    CHECK(sendCommand(MHZ19_CMD_READ_CO2, 0, 0, 0, 0, 0));

    // 16-bit CO2 value in response Bytes 2 and 3
    co2 = (ReadBuffer[2] << 8) | ReadBuffer[3];
    LastTimestamp = esp_timer_get_time();
    LastCO2Value = co2;

    return ESP_OK;
}

const char *MHZ19::getVersion() {
  if(Version[0]=='\0') {
    // Send command "Read firmware version" (NOT DOCUMENTED)
    if(sendCommand(MHZ19_CMD_GET_VERSION, 0, 0, 0, 0, 0).ok()) {
    
      // Copy 4 ASCII characters to version array like "0443"
      for (uint8_t i = 0; i < 4; i++) {
        // Version in response Bytes 2..5
        Version[i] = ReadBuffer[i + 2];
      }
    }
  }
  return &Version[0];
}

ErrorType MHZ19::setRange(MHZ19_RANGE range) {
    // Send "Set range" command
    return sendCommand(MHZ19_CMD_SET_RANGE, 0x00, 0x00, 0x00, (range >> 8), (range & 0xff));
}

ErrorType MHZ19::getRange(uint16_t &range) {
  if(Range==0xFFFF || 0==Range) {
    // Send command "Read range" (NOT DOCUMENTED)
    CHECK(sendCommand(MHZ19_CMD_GET_RANGE, 0, 0, 0, 0, 0));

    // Range is in Bytes 4 and 5
    range = (ReadBuffer[4] << 8) | ReadBuffer[5];

    // Check range according to documented specification
    if ((range != MHZ19_RANGE_2000) && (range != MHZ19_RANGE_5000))
        return ESP_ERR_INVALID_RESPONSE;

    Range = range;
  } else {
    range = Range;
  }
  return ESP_OK;
}

ErrorType MHZ19::setAutoCalibration(bool calibrationOn) {
    // Send command "Set Automatic Baseline Correction (ABC logic function)"
    return sendCommand(MHZ19_CMD_SET_AUTO_CAL, (calibrationOn ? 0xA0 : 0x00), 0, 0, 0, 0);
}

ErrorType MHZ19::getAutoCalibration(bool &calibrationOn) {
    // Send command "Get Automatic Baseline Correction (ABC logic function)" (NOT DOCUMENTED)
    CHECK(sendCommand(MHZ19_CMD_GET_AUTO_CAL, 0, 0, 0, 0, 0));

    // Response is located in Byte 7: 0 = off, 1 = on
    calibrationOn = ReadBuffer[7] & 0x01;

    return ESP_OK;
}

ErrorType MHZ19::startCalibration() {
    // Send command "Zero Point Calibration"
    return sendCommand(MHZ19_CMD_CAL_ZERO_POINT, 0, 0, 0, 0, 0);
}

ErrorType MHZ19::sendCommand(uint8_t cmd, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7) {
    uint8_t txBuffer[MHZ19_SERIAL_RX_BYTES] = { 0xFF, 0x01, cmd, b3, b4, b5, b6, b7, 0x00 };

    // Add CRC Byte
    txBuffer[8] = calcCRC(txBuffer);

    // Clear receive buffer
    uart_flush(UartPort);

#ifdef MHZ19_DEBUG
    ESP_LOGI(TAG,"sent:");
    ESP_LOG_BUFFER_HEX(TAG, &ReadBuffer[0],9);
#endif
    // Write serial data
    uart_write_bytes(UartPort, (char *) txBuffer, sizeof(txBuffer));

    // Clear receive buffer
    memset(ReadBuffer, 0, sizeof(ReadBuffer));

    // Read response from serial buffer
    int len = uart_read_bytes(UartPort, ReadBuffer, MHZ19_SERIAL_RX_BYTES, MHZ19_SERIAL_RX_TIMEOUT_MS / portTICK_RATE_MS);

#ifdef MHZ19_DEBUG
    ESP_LOGI(TAG,"uart read buffer %d",len);
    ESP_LOG_BUFFER_HEX(TAG, &ReadBuffer[0],9);
#endif

    if (len < 9)
        return ESP_ERR_TIMEOUT;

    // Check received Byte[0] == 0xFF and Byte[1] == transmit command
    if ((ReadBuffer[0] != 0xFF) || (ReadBuffer[1] != cmd)) {
        ESP_LOGE(TAG,"INVALID_RESPONSE");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Check received Byte[8] CRC
    if (ReadBuffer[8] != calcCRC(ReadBuffer)) {
        ESP_LOGE(TAG,"BAD_CRC");
        return ESP_ERR_INVALID_CRC;
    }

    // Return result
    return ESP_OK;
}

uint8_t MHZ19::calcCRC(uint8_t *data) {
    uint8_t crc = 0;

    // Calculate CRC on 8 data Bytes
    for (uint8_t i = 1; i < 8; i++)
        crc += data[i];

    crc = 0xFF - crc;
    crc++;

    // Return calculated CRC
    return crc;
}

