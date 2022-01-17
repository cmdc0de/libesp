#include "adc.h"
//#include "esp_adc_cal.h"
#include "system.h"

using namespace libesp;
const char *ADC::LOGTAG = "ADC";

ADC::ADC() : ADCChannel1(ADC1_CHANNEL_MAX), ADCChannel2(ADC2_CHANNEL_MAX), ADCWidth(ADC_WIDTH_MAX), ADCAtten(ADC_ATTEN_MAX)
             , Samples(1), VRef(1100), ADCCalCharacteristics(), VRefPin(NOPIN) {

}

adc1_channel_t ADC::convertGPIOToADC1(gpio_num_t pin) {
  switch(pin) {
    case 36:
      return ADC1_CHANNEL_0;
    case 37:
      return ADC1_CHANNEL_1;
    case 38:
      return ADC1_CHANNEL_2;
    case 39:
      return ADC1_CHANNEL_3;
    case 32:
      return ADC1_CHANNEL_4;
    case 33:
      return ADC1_CHANNEL_5;
    case 34:
      return ADC1_CHANNEL_6;
    case 35:
      return ADC1_CHANNEL_7;
    default:
      return ADC1_CHANNEL_MAX;
  }
}

adc2_channel_t ADC::convertGPIOToADC2(gpio_num_t pin) {
  switch(pin) {
    case 4:
      return ADC2_CHANNEL_0;
    case 0:
      return ADC2_CHANNEL_1;
    case 2:
      return ADC2_CHANNEL_2;
    case 15:
      return ADC2_CHANNEL_3;
    case 13:
      return ADC2_CHANNEL_4;
    case 12:
      return ADC2_CHANNEL_5;
    case 14:
      return ADC2_CHANNEL_6;
    case 27:
      return ADC2_CHANNEL_7;
    case 25:
      return ADC2_CHANNEL_8;
    case 26:
      return ADC2_CHANNEL_9;
    default:
      return ADC2_CHANNEL_MAX;
  }
}


ErrorType ADC::init(const adc1_channel_t &adc1_chan, const adc2_channel_t &adc2_chan, const adc_bits_width_t &adc_width, const adc_atten_t &atten,uint16_t samples) {
  return init(adc1_chan, adc2_chan, adc_width, atten, samples, NOPIN);
}

adc_unit_t ADC::getUnit() {
  if(ADCChannel1 != ADC1_CHANNEL_MAX)
    return ADC_UNIT_1;
  return ADC_UNIT_2;
}

ErrorType ADC::init(const adc1_channel_t &adc1_chan, const adc2_channel_t &adc2_chan, const adc_bits_width_t &adc_width, const adc_atten_t &atten , uint16_t samples, gpio_num_t refPin) {
  ErrorType et;
  ADCChannel1 = adc1_chan;
  ADCChannel2 = adc2_chan;
  ADCWidth = adc_width;
  ADCAtten = atten;
  Samples = samples;
  VRefPin = refPin;
  //only 1
  if(ADCChannel1 == ADC1_CHANNEL_MAX || ADCChannel2 == ADC2_CHANNEL_MAX) {
    if(VRefPin!=NOPIN) {
      et = adc_vref_to_gpio(ADC_UNIT_2,VRefPin);
      if(et.ok()) {
        uint32_t reading = 0;
        adc2_channel_t vrefADC = convertGPIOToADC2(VRefPin);
        for(int i=0;i<Samples;++i) {
          int raw;
          adc2_get_raw(vrefADC, ADCWidth, &raw);
          reading+=raw;
        }
        VRef = reading/Samples;
      }
    } //else check to see if value is in the fuse and update VRef
    //TODO

    if(et.ok()) {
      printEFuseData();
      printCharacteristics();
    }
  } else {
    et = ErrorType::INVALID_CONFIG;
  }
  return et;
}

uint32_t ADC::getRawSamples() {
  uint32_t adc_reading = 0;
  for (int i = 0; i < Samples; i++) {
    if (getUnit() == ADC_UNIT_1) {
      adc_reading += adc1_get_raw(ADCChannel1);
    } else {
      int raw;
      adc2_get_raw(ADCChannel2, ADCWidth, &raw);
      adc_reading += raw;
    }
  }
  return adc_reading;
}

void ADC::printEFuseData() {
  if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}

void ADC::printCharacteristics() {
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(getUnit(), ADCAtten, ADCWidth, VRef, &ADCCalCharacteristics);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    ESP_LOGI(LOGTAG, "Characterized using Two Point Value\n");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    ESP_LOGI(LOGTAG, "Characterized using eFuse Vref\n");
  } else {
    ESP_LOGI(LOGTAG,"Characterized using Default Vref\n");
  }
}

ErrorType ADC::acquireData(Result &v) {
  ErrorType et;
  uint32_t adc_reading = 0;
  //Multisampling
  for (int i = 0; i < Samples; i++) {
    if (getUnit() == ADC_UNIT_1) {
      adc_reading += adc1_get_raw(ADCChannel1);
    } else {
      int raw;
      adc2_get_raw(ADCChannel2, ADCWidth, &raw);
      adc_reading += raw;
    }
  }
  adc_reading /= Samples;
  //Convert adc_reading to voltage in mV
  uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &ADCCalCharacteristics);
  printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);
  v.RawAvg = adc_reading;
  v.CalculatedVoltage = voltage;
  return et;
}

void ADC::shutdown() {

}


ADC::~ADC() {
  shutdown();
}
