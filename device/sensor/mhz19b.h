#ifndef __LIBESP_MHZ19_H__
#define __LIBESP_MHZ19_H__

#include <driver/uart.h>
#include <driver/gpio.h>
#include "../../error_type.h"

namespace libesp {



class MHZ19 {
  public:
  /**@brief PPM range */
  enum MHZ19_RANGE {
    MHZ19_RANGE_2000 = 2000,            ///< 2000 ppm
    MHZ19_RANGE_5000 = 5000,            ///< 5000 ppm (Default)
  };
    //! 3 minutes warming-up time after power-on before valid data returned
  static const uint32_t MHZ19_WARMING_UP_TIME_MS     =(3UL * 60000UL);
  static const uint32_t MHZ19_WARMING_UP_TIME_US     =(3UL * 60000UL * 1000UL);

  //! Minimum response time between CO2 reads (EXPERIMENTALLY DEFINED)
  static const uint32_t MHZ19_READ_INTERVAL_MS       =(5UL * 1000UL);

  //! Fixed 9 Bytes response
  static const uint32_t MHZ19_SERIAL_RX_BYTES        =9;
  //! 128 is the minimal value the UART driver will accept (at least on esp32)
  static const uint32_t MHZ19_SERIAL_BUF_LEN         =128;

  //! Response timeout between 15..120 ms at 9600 baud works reliable for all commands
  static const uint32_t MHZ19_SERIAL_RX_TIMEOUT_MS   =120;

// Documented commands
  static const uint32_t MHZ19_CMD_SET_AUTO_CAL        =0x79; ///< set auto calibration on/off
  static const uint32_t MHZ19_CMD_READ_CO2            =0x86;///< read CO2 concentration
  static const uint32_t MHZ19_CMD_CAL_ZERO_POINT      =0x87;///< calibrate zero point at 400ppm
  static const uint32_t MHZ19_CMD_CAL_SPAN_PIONT      =0x88;///< calibrate span point (NOT IMPLEMENTED)
  static const uint32_t MHZ19_CMD_SET_RANGE           =0x99;///< set detection range

// Not documented commands
  static const uint32_t MHZ19_CMD_GET_AUTO_CAL        =0x7D;///< get auto calibration status (NOT DOCUMENTED)
  static const uint32_t MHZ19_CMD_GET_RANGE           =0x9B;///< get range detection (NOT DOCUMENTED)
  static const uint32_t MHZ19_CMD_GET_VERSION         =0xA0;///< get firmware version (NOT DOCUMENTED)

  public:
    MHZ19();
    ErrorType init(uart_port_t prt, gpio_num_t tx, gpio_num_t rx);
    ErrorType shutdown();
    bool detect();
    /**
    * @brief Check if sensor is warming up
     *
     * @param smart_warming_up Smart check
     *
     * @return true if sensor is warming up
     */
    bool isWarmingUp(bool smart_warming_up);
    bool isWarmingUp() {return isWarmingUp(true);}
    /**
     * @brief Check minimum interval between CO2 reads
     *
     * Not described in the datasheet, but it is the same frequency as the built-in LED blink.
     *
     * @return true if ready to call readCO2()
    */
    bool isReady();
    /**
     * @brief Read CO2 from sensor
     *
     * @param[out] co2 CO2 level
     *      - < 0: MH-Z19B response error codes.
     *      - 0..399 ppm: Incorrect values. Minimum value starts at 400ppm outdoor fresh air.
     *      - 400..1000 ppm: Concentrations typical of occupied indoor spaces with good air exchange.
     *      - 1000..2000 ppm: Complaints of drowsiness and poor air quality. Ventilation is required.
     *      - 2000..5000 ppm: Headaches, sleepiness and stagnant, stale, stuffy air. Poor concentration, loss of
     *        attention, increased heart rate and slight nausea may also be present.
     *      - Higher values are extremely dangerous and cannot be measured.
     *
     * @return ESP_OK on success
     */
    ErrorType readCO2(int16_t &co2);
    /**
     * @brief Get firmware version (NOT DOCUMENTED)
     *
     * @details
     *      This is an undocumented command, but most sensors returns ASCII "0430 or "0443".
     *
     * @param[out] version
     *      Returned character pointer to version (must be at least 5 Bytes)\n
     *      Only valid when return is set to ESP_OK.
     *
     * @return ESP_OK on success
     */
    const char *getVersion();
    /**
     * @brief Set CO2 range
     *
     * @param range Range of the sensor (2000 or 5000, in ppm)
     *
     * @return ESP_OK on success
     */
    ErrorType setRange(MHZ19_RANGE range);

    /**
     * @brief Get CO2 range in PPM (NOT DOCUMENTED)
     *
     * @details
     *      This function verifies valid read ranges of 2000 or 5000 ppm.\n
     *      Note: Other ranges may be returned, but are undocumented and marked as invalid.
     *
    * @param range Current value of the range of the sensor (output)
     *
     * @return ESP_OK on success
     */
    ErrorType getRange(uint16_t &range);

    /**
     * @brief Enable or disable automatic calibration
     *
    * @param calibration_on
     *      - true: Automatic calibration on.
     *      - false: Automatic calibration off.
     *
     * @return ESP_OK on success
     */
    ErrorType setAutoCalibration(bool calibrationOn);
    /**
    * @brief Get status of automatic calibration (NOT DOCUMENTED)
    *
    * @param[out] calibration_on Automatic calibration status
    * @return ESP_OK on success
    */
    ErrorType getAutoCalibration(bool &calibrationOn); // (NOT DOCUMENTED)

    /**
     * @brief Start Zero Point Calibration manually at 400ppm
     *
     * @details
     *      The sensor must be powered-up for at least 20 minutes in fresh air at 400ppm room
     *      temperature. Then call this function once to execute self calibration.\n
     *      Recommended to use this function when auto calibration is off.
     *
     *
     * @return ESP_OK on success
     */
    ErrorType startCalibration();
  public:
    /**
     * @brief Calculate CRC on 8 data Bytes buffer
     *
     * @param data Buffer pointer to calculate CRC.
     * @return Calculated 8-bit CRC.
     */
    static uint8_t calcCRC(uint8_t *data);
  protected:
    /**
     * @brief Send serial command to sensor and read response
     *
     * @details
     *      Send command to sensor and read response, protected with a receive timeout.\n
     *      Result is available in the device descriptor buffer.
     *
     * @param dev Pointer to the sensor device data structure
     * @param cmd Command Byte
     * @param b3 Byte 3 (default 0)
     * @param b4 Byte 4 (default 0)
     * @param b5 Byte 5 (default 0)
     * @param b6 Byte 6 (default 0)
     * @param b7 Byte 7 (default 0)
     */
    ErrorType sendCommand(uint8_t cmd, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7);
  private:
    uart_port_t UartPort;  ///< UART port used to communicate
    uint8_t ReadBuffer[MHZ19_SERIAL_BUF_LEN];           ///< read buffer attached to this device
    int16_t LastCO2Value;     ///< last read value
    int64_t LastTimestamp;        ///< timestamp of the last sensor co2 level reading
    char    Version[5];
};

}
#endif

