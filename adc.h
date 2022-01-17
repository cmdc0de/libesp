#pragma once

#include "error_type.h"
#include <driver/gpio.h>
#include <driver/adc.h>
#include <hal/adc_types.h>
#include <esp_adc_cal.h>

namespace libesp {

class ADC {
public:
  static const char *LOGTAG;
  struct Result {
    uint32_t RawAvg;
    uint32_t CalculatedVoltage;
  };
  static adc1_channel_t convertGPIOToADC1(gpio_num_t pin);
  static adc2_channel_t convertGPIOToADC2(gpio_num_t pin);
public:
  ADC();
  ErrorType init(const adc1_channel_t &adc1_chan, const adc2_channel_t &adc2_chan, const adc_bits_width_t &adc_width, const adc_atten_t &atten, uint16_t samples);
  ErrorType init(const adc1_channel_t &adc1_chan, const adc2_channel_t &adc2_chan, const adc_bits_width_t &adc_width, const adc_atten_t &atten, uint16_t samples, gpio_num_t refPin);
  ErrorType acquireData(Result &v);
  void shutdown();
  uint32_t getRawSamples();
  ~ADC();
protected:
  void printEFuseData();
  void printCharacteristics();
  adc_unit_t getUnit();
private:
  adc1_channel_t ADCChannel1;
  adc2_channel_t ADCChannel2;
  adc_bits_width_t ADCWidth;
  adc_atten_t ADCAtten;
  uint16_t Samples;
  uint32_t VRef;
  esp_adc_cal_characteristics_t ADCCalCharacteristics;
  gpio_num_t VRefPin;
};

}
