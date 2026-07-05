// VL53L0X Time-of-Flight distance sensor driver.
//
// Implements ST's initialization sequence following their UM2039 reference
// implementation (also known as the Pololu VL53L0X port).  All I2C traffic
// goes through the project's libesp::I2CBus / libesp::I2CDevice wrappers
// (ESP-IDF i2c_master driver).

#include "vl53l0x.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>

// ---------------------------------------------------------------------------
// VL53L0X register map (subset used by this driver)
// ---------------------------------------------------------------------------
namespace {

// System / ranging control
constexpr uint8_t REG_SYSRANGE_START                         = 0x00;
constexpr uint8_t REG_SYSTEM_SEQUENCE_CONFIG                 = 0x01;
constexpr uint8_t REG_SYSTEM_INTERMEASUREMENT_PERIOD         = 0x04;
constexpr uint8_t REG_SYSTEM_INTERRUPT_CONFIG_GPIO           = 0x0A;
constexpr uint8_t REG_SYSTEM_INTERRUPT_CLEAR                 = 0x0B;

// Result registers
constexpr uint8_t REG_RESULT_INTERRUPT_STATUS                = 0x13;
constexpr uint8_t REG_RESULT_RANGE_STATUS                    = 0x14;  // +10 = range mm (2 bytes)

// GPIO / analog
constexpr uint8_t REG_GPIO_HV_MUX_ACTIVE_HIGH                = 0x84;

// MSRC (minimum signal rate check)
constexpr uint8_t REG_MSRC_CONFIG_CONTROL                    = 0x60;
constexpr uint8_t REG_MSRC_CONFIG_TIMEOUT_MACROP             = 0x46;

// Pre-range VCSEL & timeout
constexpr uint8_t REG_PRE_RANGE_CONFIG_VCSEL_PERIOD          = 0x50;
constexpr uint8_t REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI     = 0x51;
constexpr uint8_t REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO     = 0x52;

// Final-range VCSEL & timeout
constexpr uint8_t REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD        = 0x70;
constexpr uint8_t REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI   = 0x71;
constexpr uint8_t REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO   = 0x72;

// Identification
constexpr uint8_t REG_IDENTIFICATION_MODEL_ID                = 0xC0;  // expected 0xEE
constexpr uint8_t REG_IDENTIFICATION_REVISION_ID             = 0xC2;  // expected 0x10

// I2C address change
constexpr uint8_t REG_I2C_SLAVE_DEVICE_ADDRESS               = 0x8A;

// SPAD management
constexpr uint8_t REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0       = 0xB0;
constexpr uint8_t REG_GLOBAL_CONFIG_REF_EN_START_SELECT      = 0xB6;
constexpr uint8_t REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD    = 0x4E;
constexpr uint8_t REG_DYNAMIC_SPAD_REF_EN_START_OFFSET       = 0x4F;
constexpr uint8_t REG_POWER_MANAGEMENT_GO1_POWER_FORCE       = 0x80;

// Expected model ID
constexpr uint8_t MODEL_ID_EXPECTED = 0xEE;

// Timing-budget overhead constants (µs) from ST's reference driver
constexpr uint32_t TB_OVERHEAD_START_END   = 1320;
constexpr uint32_t TB_OVERHEAD_END         =  960;
constexpr uint32_t TB_OVERHEAD_MSRC        =  660;
constexpr uint32_t TB_OVERHEAD_TCC         =  590;
constexpr uint32_t TB_OVERHEAD_DSS1        =  690;
constexpr uint32_t TB_OVERHEAD_DSS2        =  420;
constexpr uint32_t TB_OVERHEAD_PRE_RANGE   =  660;
constexpr uint32_t TB_OVERHEAD_FINAL_RANGE = 550;

// Minimum achievable timing budget (µs)
constexpr uint32_t TB_MIN_US = 20000;

// Per-transaction I2C timeout (ms)
constexpr int I2C_TIMEOUT_MS = 25;

} // anonymous namespace

// ---------------------------------------------------------------------------

const char *VL53L0X::LOGTAG = "VL53L0X";

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

VL53L0X::VL53L0X(gpio_num_t xshunt)
    : Bus(nullptr)
    , Device(nullptr)
    , Address(DEFAULT_ADDRESS)
    , StopVariable(0)
    , MeasurementTimingBudgetUs(33000)
	 , xShunt(xshunt)
    , ContinuousMode(false)
    , Initialized(false)
{}

VL53L0X::~VL53L0X() {
    shutdown();
}

void VL53L0X::shutdown() {
    Initialized    = false;
    ContinuousMode = false;
    if (Device) {
        delete Device;  // detaches from the bus via ~I2CMaster
        Device = nullptr;
    }
    Bus = nullptr;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::init(libesp::I2CBus *bus, uint8_t address) {
    libesp::ErrorType err;

    if (bus == nullptr) {
        ESP_LOGE(LOGTAG, "null I2C bus");
        err = ESP_ERR_INVALID_ARG;
        return err;
    }

    Bus     = bus;
    Address = address;

	 if ( xShunt != GPIO_NUM_NC) {
        gpio_reset_pin(xShunt);
        gpio_set_level(xShunt, 0);
        gpio_set_drive_capability(xShunt, GPIO_DRIVE_CAP_3);
        gpio_set_direction(xShunt, GPIO_MODE_OUTPUT);
        gpio_set_level(xShunt, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(xShunt, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
	 }

    i2c_device_config_t devcfg = {};
    devcfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    devcfg.device_address  = Address;
    devcfg.scl_speed_hz    = DEFAULT_I2C_CLOCK_HZ;
    Device = Bus->createMasterDevice(devcfg);
    if (!Device) {
        ESP_LOGE(LOGTAG, "failed to create I2C device at 0x%02X", Address);
        err = ESP_ERR_NO_MEM;
        return err;
    }

    // --- 1. Verify device identity ----------------------------------------
    uint8_t modelId = 0;
    err = readReg(REG_IDENTIFICATION_MODEL_ID, modelId);
    if (!err.ok()) {
        ESP_LOGE(LOGTAG, "failed to read model ID: %s", err.toString());
        return err;
    }
    if (modelId != MODEL_ID_EXPECTED) {
        ESP_LOGE(LOGTAG, "unexpected model ID 0x%02X (expected 0x%02X)", modelId, MODEL_ID_EXPECTED);
        err = ESP_ERR_NOT_FOUND;
        return err;
    }
    ESP_LOGI(LOGTAG, "VL53L0X found (model ID 0x%02X)", modelId);

    // --- 2. ST reference driver init sequence -----------------------------
    // (a) Set I2C standard mode
    err = writeReg(0x88, 0x00);  if (!err.ok()) return err;
    err = writeReg(0x80, 0x01);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x01);  if (!err.ok()) return err;
    err = writeReg(0x00, 0x00);  if (!err.ok()) return err;

    err = readReg(0x91, StopVariable);
    if (!err.ok()) {
        ESP_LOGE(LOGTAG, "failed to read stop variable: %s", err.toString());
        return err;
    }

    err = writeReg(0x00, 0x01);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x00);  if (!err.ok()) return err;
    err = writeReg(0x80, 0x00);  if (!err.ok()) return err;

    // (b) Disable SIGNAL_RATE_MSRC and SIGNAL_RATE_PRE_RANGE limit checks
    uint8_t msrcCfg = 0;
    err = readReg(REG_MSRC_CONFIG_CONTROL, msrcCfg);
    if (!err.ok()) return err;
    err = writeReg(REG_MSRC_CONFIG_CONTROL, msrcCfg | 0x12);
    if (!err.ok()) return err;

    // (c) Set final range signal rate limit to 0.25 MCPS (encoded as 16.16 fixed-point)
    err = writeReg16(0x44, 0x0020);
    if (!err.ok()) return err;

    // --- 3. Load tuning settings ------------------------------------------
    err = loadTuningSettings();
    if (!err.ok()) {
        ESP_LOGE(LOGTAG, "loadTuningSettings: %s", err.toString());
        return err;
    }

    // --- 4. Configure GPIO interrupt: new sample ready --------------------
    err = writeReg(REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    if (!err.ok()) return err;

    uint8_t gpioHvMux = 0;
    err = readReg(REG_GPIO_HV_MUX_ACTIVE_HIGH, gpioHvMux);
    if (!err.ok()) return err;
    err = writeReg(REG_GPIO_HV_MUX_ACTIVE_HIGH, gpioHvMux & ~0x10);
    if (!err.ok()) return err;
    err = writeReg(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    if (!err.ok()) return err;

    // --- 5. Capture current timing budget (set by ST tuning) -------------
    {
        bool tcc, dss, msrc, preRange, finalRange;
        err = getSequenceStepEnables(tcc, dss, msrc, preRange, finalRange);
        if (!err.ok()) return err;

        uint16_t preVcsel = 0, finalVcsel = 0;
        uint32_t msrcUs = 0, preUs = 0, finalUs = 0;
        err = getSequenceStepTimeouts(preRange, preVcsel, msrcUs, preUs, finalUs, finalVcsel);
        if (!err.ok()) return err;

        uint32_t usedBudget = TB_OVERHEAD_START_END;
        if (tcc)        usedBudget += msrcUs + TB_OVERHEAD_TCC;
        if (dss)        usedBudget += 2 * (msrcUs + TB_OVERHEAD_DSS1) + TB_OVERHEAD_DSS2;
        else if (msrc)  usedBudget += msrcUs + TB_OVERHEAD_MSRC;
        if (preRange)   usedBudget += preUs  + TB_OVERHEAD_PRE_RANGE;
        if (finalRange) usedBudget += finalUs + TB_OVERHEAD_FINAL_RANGE;
        usedBudget += TB_OVERHEAD_END;

        MeasurementTimingBudgetUs = usedBudget;
    }

    // --- 6. SPAD calibration ---------------------------------------------
    {
        uint8_t spadCount = 0;
        bool    isAperture = false;
        err = getSpadInfo(spadCount, isAperture);
        if (!err.ok()) {
            ESP_LOGE(LOGTAG, "getSpadInfo: %s", err.toString());
            return err;
        }
        ESP_LOGI(LOGTAG, "SPAD count=%u aperture=%d", spadCount, (int)isAperture);

        uint8_t refSpadMap[6];
        err = readMulti(REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, refSpadMap, 6);
        if (!err.ok()) return err;

        err = writeReg(0xFF, 0x01);  if (!err.ok()) return err;
        err = writeReg(REG_DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00); if (!err.ok()) return err;
        err = writeReg(REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C); if (!err.ok()) return err;
        err = writeReg(0xFF, 0x00);  if (!err.ok()) return err;
        err = writeReg(REG_GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4); if (!err.ok()) return err;

        uint8_t firstSpadToEnable = isAperture ? 12 : 0;
        uint8_t spadsEnabled = 0;
        for (uint8_t i = 0; i < 48; ++i) {
            if (i < firstSpadToEnable || spadsEnabled == spadCount) {
                refSpadMap[i / 8] &= ~(1 << (i % 8));
            } else if (refSpadMap[i / 8] & (1 << (i % 8))) {
                ++spadsEnabled;
            }
        }
        err = writeMulti(REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, refSpadMap, 6);
        if (!err.ok()) return err;
    }

    // --- 7. Re-load tuning settings (ST reference does this twice) --------
    err = loadTuningSettings();
    if (!err.ok()) return err;

    // --- 8. Sequence steps and calibration --------------------------------
    err = writeReg(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8); if (!err.ok()) return err;
    err = setMeasurementTimingBudget(MeasurementTimingBudgetUs);  if (!err.ok()) return err;

    err = writeReg(REG_SYSTEM_SEQUENCE_CONFIG, 0x01); if (!err.ok()) return err;
    err = performSingleRefCalibration(0x40);          if (!err.ok()) return err;
    err = writeReg(REG_SYSTEM_SEQUENCE_CONFIG, 0x02); if (!err.ok()) return err;
    err = performSingleRefCalibration(0x00);          if (!err.ok()) return err;
    // Restore standard sequence
    err = writeReg(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8); if (!err.ok()) return err;

    Initialized = true;
    ESP_LOGI(LOGTAG, "init complete, timing budget %lu us", (unsigned long)MeasurementTimingBudgetUs);
    return err;
}

// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::read(RangingData &data, uint32_t timeoutMs) {
    libesp::ErrorType err;
    if (!Initialized) {
        err = ESP_ERR_INVALID_STATE;
        return err;
    }

    if (!ContinuousMode) {
        // Trigger a single measurement
        err = writeReg(0x80, 0x01);  if (!err.ok()) return err;
        err = writeReg(0xFF, 0x01);  if (!err.ok()) return err;
        err = writeReg(0x00, 0x00);  if (!err.ok()) return err;
        err = writeReg(0x91, StopVariable); if (!err.ok()) return err;
        err = writeReg(0x00, 0x01);  if (!err.ok()) return err;
        err = writeReg(0xFF, 0x00);  if (!err.ok()) return err;
        err = writeReg(0x80, 0x00);  if (!err.ok()) return err;
        err = writeReg(REG_SYSRANGE_START, 0x01);
        if (!err.ok()) return err;

        // Wait for start bit to clear
        TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeoutMs);
        uint8_t sysRange = 0;
        do {
            err = readReg(REG_SYSRANGE_START, sysRange);
            if (!err.ok()) return err;
            if (!(sysRange & 0x01)) break;
            vTaskDelay(1);
        } while (xTaskGetTickCount() < deadline);

        if (sysRange & 0x01) {
            err = libesp::ErrorType::TIMEOUT_ERROR;
            ESP_LOGE(LOGTAG, "timeout waiting for start bit to clear");
            return err;
        }
    }

    // Poll interrupt status until measurement ready (bits 2:0 set)
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeoutMs);
    uint8_t intStatus = 0;
    do {
        err = readReg(REG_RESULT_INTERRUPT_STATUS, intStatus);
        if (!err.ok()) return err;
        if (intStatus & 0x07) break;
        vTaskDelay(1);
    } while (xTaskGetTickCount() < deadline);

    if (!(intStatus & 0x07)) {
        err = libesp::ErrorType::TIMEOUT_ERROR;
        ESP_LOGE(LOGTAG, "timeout waiting for measurement ready");
        return err;
    }

    // Read range result: REG_RESULT_RANGE_STATUS + 10 (2 bytes, big-endian)
    uint8_t resultBuf[12];
    err = readMulti(REG_RESULT_RANGE_STATUS, resultBuf, 12);
    if (!err.ok()) return err;

    data.rangeMillimeters = (static_cast<uint16_t>(resultBuf[10]) << 8) | resultBuf[11];

    // Bits 6:3 hold ST's internal device range status; 11 = "range complete"
    // (the only valid value — ST's API maps it to PAL status 0). The sensor
    // reports ~8190 mm when nothing is in range even with status 11.
    uint8_t deviceStatus = (resultBuf[0] & 0x78) >> 3;
    if (deviceStatus == 11 && data.rangeMillimeters < 8190) {
        data.rangeStatus = 0;
    } else {
        data.rangeStatus = (deviceStatus == 0) ? 0xFF : deviceStatus;
    }

    // Clear interrupt
    err = writeReg(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    if (!err.ok()) return err;

    if (!ContinuousMode) {
        // Put sensor back to idle
        err = writeReg(REG_SYSRANGE_START, 0x00);
    }

    return err;
}

// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::startContinuous(uint32_t periodMs) {
    libesp::ErrorType err;
    if (!Initialized) { err = ESP_ERR_INVALID_STATE; return err; }

    err = writeReg(0x80, 0x01);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x01);  if (!err.ok()) return err;
    err = writeReg(0x00, 0x00);  if (!err.ok()) return err;
    err = writeReg(0x91, StopVariable); if (!err.ok()) return err;
    err = writeReg(0x00, 0x01);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x00);  if (!err.ok()) return err;
    err = writeReg(0x80, 0x00);  if (!err.ok()) return err;

    if (periodMs != 0) {
        // Timed mode: write the period register (unit: ms, stored as (period - 1))
        uint16_t oscCal = 0;
        err = readReg16(0xF8, oscCal);
        if (!err.ok()) return err;

        uint32_t periodReg = static_cast<uint32_t>(oscCal) * periodMs;
        err = writeReg16(REG_SYSTEM_INTERMEASUREMENT_PERIOD,
                         static_cast<uint16_t>(periodReg >> 6));
        if (!err.ok()) return err;
        err = writeReg(REG_SYSRANGE_START, 0x04);  // continuous timed
    } else {
        err = writeReg(REG_SYSRANGE_START, 0x02);  // continuous back-to-back
    }

    if (err.ok()) {
        ContinuousMode = true;
        ESP_LOGI(LOGTAG, "continuous ranging started (period %lu ms)", (unsigned long)periodMs);
    }
    return err;
}

// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::stopContinuous() {
    libesp::ErrorType err;
    if (!Initialized) { err = ESP_ERR_INVALID_STATE; return err; }

    err = writeReg(REG_SYSRANGE_START, 0x01);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x01);  if (!err.ok()) return err;
    err = writeReg(0x00, 0x00);  if (!err.ok()) return err;
    err = writeReg(0x91, 0x00);  if (!err.ok()) return err;
    err = writeReg(0x00, 0x01);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x00);  if (!err.ok()) return err;

    ContinuousMode = false;
    ESP_LOGI(LOGTAG, "continuous ranging stopped");
    return err;
}

// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::setAddress(uint8_t newAddress) {
    libesp::ErrorType err = writeReg(REG_I2C_SLAVE_DEVICE_ADDRESS, newAddress & 0x7F);
    if (err.ok()) {
        Address = newAddress & 0x7F;
        // The bus device handle is bound to the old address — re-create it
        delete Device;
        Device = nullptr;
        i2c_device_config_t devcfg = {};
        devcfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        devcfg.device_address  = Address;
        devcfg.scl_speed_hz    = DEFAULT_I2C_CLOCK_HZ;
        Device = Bus->createMasterDevice(devcfg);
        if (!Device) {
            ESP_LOGE(LOGTAG, "failed to re-create I2C device at 0x%02X", Address);
            err = ESP_ERR_NO_MEM;
            return err;
        }
        ESP_LOGI(LOGTAG, "I2C address changed to 0x%02X", Address);
    }
    return err;
}

// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::setMeasurementTimingBudget(uint32_t budgetUs) {
    if (budgetUs < TB_MIN_US) {
        ESP_LOGE(LOGTAG, "timing budget %lu us < minimum %lu us",
                 (unsigned long)budgetUs, (unsigned long)TB_MIN_US);
        return libesp::ErrorType(libesp::ErrorType::INVALID_PARAM);
    }

    bool tcc, dss, msrc, preRange, finalRange;
    libesp::ErrorType err = getSequenceStepEnables(tcc, dss, msrc, preRange, finalRange);
    if (!err.ok()) return err;

    uint16_t preVcsel = 0, finalVcsel = 0;
    uint32_t msrcUs = 0, preUs = 0, finalUs = 0;
    err = getSequenceStepTimeouts(preRange, preVcsel, msrcUs, preUs, finalUs, finalVcsel);
    if (!err.ok()) return err;

    uint32_t usedBudget = TB_OVERHEAD_START_END;
    if (tcc)        usedBudget += msrcUs + TB_OVERHEAD_TCC;
    if (dss)        usedBudget += 2 * (msrcUs + TB_OVERHEAD_DSS1) + TB_OVERHEAD_DSS2;
    else if (msrc)  usedBudget += msrcUs + TB_OVERHEAD_MSRC;
    if (preRange)   usedBudget += preUs  + TB_OVERHEAD_PRE_RANGE;
    usedBudget += TB_OVERHEAD_END;

    if (usedBudget > budgetUs) {
        ESP_LOGE(LOGTAG, "requested budget %lu us too small (overhead %lu us)",
                 (unsigned long)budgetUs, (unsigned long)usedBudget);
        return libesp::ErrorType(libesp::ErrorType::INVALID_PARAM);
    }

    if (finalRange) {
        uint32_t finalUs = budgetUs - usedBudget - TB_OVERHEAD_FINAL_RANGE;
        uint32_t finalMclks = timeoutMicrosecondsToMclks(finalUs, finalVcsel);
        uint16_t encoded = encodeTimeout(finalMclks);
        err = writeReg(REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, (encoded >> 8) & 0xFF);
        if (!err.ok()) return err;
        err = writeReg(REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO, encoded & 0xFF);
        if (!err.ok()) return err;
    }

    MeasurementTimingBudgetUs = budgetUs;
    return err;
}

uint32_t VL53L0X::getMeasurementTimingBudget() const {
    return MeasurementTimingBudgetUs;
}

// ---------------------------------------------------------------------------
// Protected: low-level I2C register I/O
// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::writeReg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    return Device->transmit(buf, 2, I2C_TIMEOUT_MS);
}

libesp::ErrorType VL53L0X::writeReg16(uint8_t reg, uint16_t value) {
    uint8_t buf[3] = {
        reg,
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF)
    };
    return Device->transmit(buf, 3, I2C_TIMEOUT_MS);
}

libesp::ErrorType VL53L0X::writeMulti(uint8_t reg, const uint8_t *buf, uint8_t len) {
    // Register address then data bytes in one transaction, no copying
    i2c_master_transmit_multi_buffer_info_t buffers[2] = {
        {.write_buffer = &reg, .buffer_size = 1},
        {.write_buffer = const_cast<uint8_t*>(buf), .buffer_size = len}
    };
    return Device->transmitMultiBuffer(buffers, 2, I2C_TIMEOUT_MS);
}

libesp::ErrorType VL53L0X::readReg(uint8_t reg, uint8_t &value) {
    return Device->transmitReceive(&reg, 1, &value, 1, I2C_TIMEOUT_MS);
}

libesp::ErrorType VL53L0X::readReg16(uint8_t reg, uint16_t &value) {
    uint8_t buf[2] = {0, 0};
    libesp::ErrorType err = Device->transmitReceive(&reg, 1, buf, 2, I2C_TIMEOUT_MS);
    if (err.ok()) {
        value = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
    }
    return err;
}

libesp::ErrorType VL53L0X::readMulti(uint8_t reg, uint8_t *buf, uint8_t len) {
    return Device->transmitReceive(&reg, 1, buf, len, I2C_TIMEOUT_MS);
}

// ---------------------------------------------------------------------------
// Protected: initialization helpers
// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::loadTuningSettings() {
    // From ST UM2039 / Pololu VL53L0X reference implementation.
    // These magic register writes configure internal sensor analog parameters.
    static const uint8_t tuning[][2] = {
        {0xFF, 0x01}, {0x00, 0x00}, {0xFF, 0x00}, {0x09, 0x00},
        {0x10, 0x00}, {0x11, 0x00}, {0x24, 0x01}, {0x25, 0xFF},
        {0x75, 0x00}, {0xFF, 0x01}, {0x4E, 0x2C}, {0x48, 0x00},
        {0x30, 0x20}, {0xFF, 0x00}, {0x30, 0x09}, {0x54, 0x00},
        {0x31, 0x04}, {0x32, 0x03}, {0x40, 0x83}, {0x46, 0x25},
        {0x60, 0x00}, {0x27, 0x00}, {0x50, 0x06}, {0x51, 0x00},
        {0x52, 0x96}, {0x56, 0x08}, {0x57, 0x30}, {0x61, 0x00},
        {0x62, 0x00}, {0x64, 0x00}, {0x65, 0x00}, {0x66, 0xA0},
        {0xFF, 0x01}, {0x22, 0x32}, {0x47, 0x14}, {0x49, 0xFF},
        {0x4A, 0x00}, {0xFF, 0x00}, {0x7A, 0x0A}, {0x7B, 0x00},
        {0x78, 0x21}, {0xFF, 0x01}, {0x23, 0x34}, {0x42, 0x00},
        {0x44, 0xFF}, {0x45, 0x26}, {0x46, 0x05}, {0x40, 0x40},
        {0x0E, 0x06}, {0x20, 0x1A}, {0x43, 0x40}, {0xFF, 0x00},
        {0x34, 0x03}, {0x35, 0x44}, {0xFF, 0x01}, {0x31, 0x04},
        {0x4B, 0x09}, {0x4C, 0x05}, {0x4D, 0x04}, {0xFF, 0x00},
        {0x44, 0x00}, {0x45, 0x20}, {0x47, 0x08}, {0x48, 0x28},
        {0x67, 0x00}, {0x70, 0x04}, {0x71, 0x01}, {0x72, 0xFE},
        {0x76, 0x00}, {0x77, 0x00}, {0xFF, 0x01}, {0x0D, 0x01},
        {0xFF, 0x00}, {0x80, 0x01}, {0x01, 0xF8}, {0xFF, 0x01},
        {0x8E, 0x01}, {0x00, 0x01}, {0xFF, 0x00}, {0x80, 0x00},
    };

    libesp::ErrorType err;
    for (const auto &pair : tuning) {
        err = writeReg(pair[0], pair[1]);
        if (!err.ok()) {
            ESP_LOGE(LOGTAG, "loadTuningSettings writeReg(0x%02X) failed: %s",
                     pair[0], err.toString());
            return err;
        }
    }
    return err;
}

// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::getSpadInfo(uint8_t &count, bool &isAperture) {
    libesp::ErrorType err;
    uint8_t tmp = 0;

    err = writeReg(0x80, 0x01);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x01);  if (!err.ok()) return err;
    err = writeReg(0x00, 0x00);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x06);  if (!err.ok()) return err;

    err = readReg(0x83, tmp);    if (!err.ok()) return err;
    err = writeReg(0x83, tmp | 0x04); if (!err.ok()) return err;

    err = writeReg(0xFF, 0x07);  if (!err.ok()) return err;
    err = writeReg(0x81, 0x01);  if (!err.ok()) return err;
    err = writeReg(0x80, 0x01);  if (!err.ok()) return err;
    err = writeReg(0x94, 0x6B);  if (!err.ok()) return err;
    err = writeReg(0x83, 0x00);  if (!err.ok()) return err;

    // Poll until 0x83 becomes non-zero
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(500);
    do {
        err = readReg(0x83, tmp);
        if (!err.ok()) return err;
        if (tmp != 0x00) break;
        vTaskDelay(1);
    } while (xTaskGetTickCount() < deadline);

    if (tmp == 0x00) {
        err = libesp::ErrorType::TIMEOUT_ERROR;
        return err;
    }

    err = writeReg(0x83, 0x01);  if (!err.ok()) return err;
    err = readReg(0x92, tmp);    if (!err.ok()) return err;

    count      = tmp & 0x7F;
    isAperture = (tmp >> 7) & 0x01;

    err = writeReg(0x81, 0x00);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x06);  if (!err.ok()) return err;

    err = readReg(0x83, tmp);    if (!err.ok()) return err;
    err = writeReg(0x83, tmp & ~0x04); if (!err.ok()) return err;

    err = writeReg(0xFF, 0x01);  if (!err.ok()) return err;
    err = writeReg(0x00, 0x01);  if (!err.ok()) return err;
    err = writeReg(0xFF, 0x00);  if (!err.ok()) return err;
    err = writeReg(0x80, 0x00);  if (!err.ok()) return err;

    return err;
}

// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::performSingleRefCalibration(uint8_t vhvInitByte) {
    libesp::ErrorType err;
    err = writeReg(REG_SYSRANGE_START, 0x01 | vhvInitByte);
    if (!err.ok()) return err;

    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(500);
    uint8_t intStatus = 0;
    do {
        err = readReg(REG_RESULT_INTERRUPT_STATUS, intStatus);
        if (!err.ok()) return err;
        if (intStatus & 0x07) break;
        vTaskDelay(1);
    } while (xTaskGetTickCount() < deadline);

    if (!(intStatus & 0x07)) {
        err = libesp::ErrorType::TIMEOUT_ERROR;
        ESP_LOGE(LOGTAG, "performSingleRefCalibration timeout");
        return err;
    }

    err = writeReg(REG_SYSTEM_INTERRUPT_CLEAR, 0x01); if (!err.ok()) return err;
    err = writeReg(REG_SYSRANGE_START, 0x00);
    return err;
}

// ---------------------------------------------------------------------------
// Protected: timing-budget helpers
// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::getSequenceStepEnables(
    bool &tcc, bool &dss, bool &msrc, bool &preRange, bool &finalRange)
{
    uint8_t seqCfg = 0;
    libesp::ErrorType err = readReg(REG_SYSTEM_SEQUENCE_CONFIG, seqCfg);
    if (!err.ok()) return err;

    tcc        = (seqCfg >> 4) & 0x01;
    dss        = (seqCfg >> 3) & 0x01;
    msrc       = (seqCfg >> 2) & 0x01;
    preRange   = (seqCfg >> 6) & 0x01;
    finalRange = (seqCfg >> 7) & 0x01;
    return err;
}

// ---------------------------------------------------------------------------

libesp::ErrorType VL53L0X::getSequenceStepTimeouts(
    bool preRangeEnabled,
    uint16_t &preRangeVcselPeriodPclks,
    uint32_t &msrcDssTccUs,
    uint32_t &preRangeUs,
    uint32_t &finalRangeUs,
    uint16_t &finalRangeVcselPeriodPclks)
{
    libesp::ErrorType err;
    uint8_t vcselReg = 0;

    // Pre-range VCSEL period
    err = readReg(REG_PRE_RANGE_CONFIG_VCSEL_PERIOD, vcselReg);
    if (!err.ok()) return err;
    preRangeVcselPeriodPclks = (vcselReg + 1) << 1;

    // MSRC timeout
    uint8_t msrcTimeout = 0;
    err = readReg(REG_MSRC_CONFIG_TIMEOUT_MACROP, msrcTimeout);
    if (!err.ok()) return err;
    msrcDssTccUs = timeoutMclksToMicroseconds(
        decodeTimeout(msrcTimeout + 1), preRangeVcselPeriodPclks);

    // Pre-range timeout
    uint8_t preHi = 0, preLo = 0;
    err = readReg(REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, preHi); if (!err.ok()) return err;
    err = readReg(REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO, preLo); if (!err.ok()) return err;
    preRangeUs = timeoutMclksToMicroseconds(
        decodeTimeout((static_cast<uint16_t>(preHi) << 8) | preLo),
        preRangeVcselPeriodPclks);

    // Final-range VCSEL period
    err = readReg(REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD, vcselReg);
    if (!err.ok()) return err;
    finalRangeVcselPeriodPclks = (vcselReg + 1) << 1;

    // Final-range timeout
    uint8_t finHi = 0, finLo = 0;
    err = readReg(REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, finHi); if (!err.ok()) return err;
    err = readReg(REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO, finLo); if (!err.ok()) return err;
    uint16_t finRaw = (static_cast<uint16_t>(finHi) << 8) | finLo;

    // Final-range timeout includes pre-range overhead when pre-range is enabled
    uint32_t finMclks = decodeTimeout(finRaw);
    if (preRangeEnabled) {
        uint32_t preRangeMclks = timeoutMicrosecondsToMclks(preRangeUs, preRangeVcselPeriodPclks);
        finMclks = (finMclks > preRangeMclks) ? finMclks - preRangeMclks : 0;
    }
    finalRangeUs = timeoutMclksToMicroseconds(
        static_cast<uint16_t>(finMclks), finalRangeVcselPeriodPclks);

    return err;
}

// ---------------------------------------------------------------------------

uint32_t VL53L0X::timeoutMclksToMicroseconds(uint16_t mclks,
                                              uint8_t  vcselPeriodPclks) const {
    // Macro period (ns) = 2304 * vcsel_pclks * 1655ps, per ST's calcMacroPeriod
    uint32_t macroPeriodNs = (2304UL * vcselPeriodPclks * 1655UL + 500UL) / 1000UL;
    return (static_cast<uint32_t>(mclks) * macroPeriodNs + 500UL) / 1000UL;
}

uint32_t VL53L0X::timeoutMicrosecondsToMclks(uint32_t us,
                                              uint8_t  vcselPeriodPclks) const {
    uint32_t macroPeriodNs = (2304UL * vcselPeriodPclks * 1655UL + 500UL) / 1000UL;
    return (us * 1000UL + macroPeriodNs / 2) / macroPeriodNs;
}

uint16_t VL53L0X::decodeTimeout(uint16_t regVal) const {
    // Format: (lsb * 2^msb) + 1
    return static_cast<uint16_t>(
        (static_cast<uint32_t>(regVal & 0x00FF) << ((regVal >> 8) & 0xFF)) + 1);
}

uint16_t VL53L0X::encodeTimeout(uint32_t mclks) const {
    if (mclks == 0) return 0;
    uint32_t lsb = mclks - 1;
    uint16_t msb = 0;
    while (lsb > 0xFF) { lsb >>= 1; ++msb; }
    return static_cast<uint16_t>((msb << 8) | (lsb & 0xFF));
}

// ---------------------------------------------------------------------------
// VL53L0XTask — measures the front sensor from its own task
// ---------------------------------------------------------------------------

const char *VL53L0XTask::LOGTAG = "VL53L0XTask";

VL53L0XTask::VL53L0XTask(gpio_num_t xshunt)
    : Task("frontTOF", 4096, 5)
    , m_sensor(xshunt)
    , m_intervalMs(DEFAULT_INTERVAL_MS)
    , m_distanceMm(INVALID_DISTANCE_MM)
    , m_running(false)
    , m_stopped(true)
    , m_initialized(false)
{}

VL53L0XTask::~VL53L0XTask() {
    shutdown();
}

libesp::ErrorType VL53L0XTask::init(libesp::I2CBus *bus, uint8_t address,
                                    uint32_t measureIntervalMs) {
    if (m_initialized) {
        ESP_LOGW(LOGTAG, "Already initialised");
        return libesp::ErrorType(ESP_OK);
    }

    m_intervalMs = measureIntervalMs;

    libesp::ErrorType et = m_sensor.init(bus, address);
    if (!et.ok()) {
        return et;
    }

    m_initialized = true;
    ESP_LOGI(LOGTAG, "Init OK — interval=%" PRIu32 " ms", m_intervalMs);
    return et;
}

libesp::ErrorType VL53L0XTask::shutdown() {
    if (!m_initialized) {
        return libesp::ErrorType(ESP_OK);
    }

    // Stop the loop and wait for it to exit before releasing the sensor,
    // so read() can't touch a detached I2C device.
    m_running.store(false);
    while (!m_stopped.load()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    m_sensor.shutdown();
    m_distanceMm.store(INVALID_DISTANCE_MM, std::memory_order_relaxed);
    m_initialized = false;
    ESP_LOGI(LOGTAG, "Shutdown complete");
    return libesp::ErrorType(ESP_OK);
}

void VL53L0XTask::run(void* /*data*/) {
    if (!m_initialized) {
        ESP_LOGE(LOGTAG, "Task started before init() — aborting");
        return;
    }

    m_running.store(true);
    m_stopped.store(false);
    ESP_LOGI(LOGTAG, "Measurement task running");

    TickType_t lastWake = xTaskGetTickCount();
    uint32_t count = 0;
    while (m_running.load()) {
        VL53L0X::RangingData d;
        libesp::ErrorType et = m_sensor.read(d, READ_TIMEOUT_MS);

        if (et.ok() && d.isValid()) {
            m_distanceMm.store(d.rangeMillimeters, std::memory_order_relaxed);
            // Log at most one reading per second; measurements run at 10 Hz
            if ((count++ % 10) == 0) {
                ESP_LOGI(LOGTAG, "%u mm", d.rangeMillimeters);
            }
        } else {
            m_distanceMm.store(INVALID_DISTANCE_MM, std::memory_order_relaxed);
            if ((count++ % 10) == 0) {
                ESP_LOGI(LOGTAG, "no valid reading (status=%u, raw=%u mm, err=%s)",
                         d.rangeStatus, d.rangeMillimeters, et.toString());
            }
        }

        // Fixed-rate pacing: the measurement itself (~33 ms budget) counts
        // against the interval, keeping the rate at 1/m_intervalMs.
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(m_intervalMs));
    }

    ESP_LOGI(LOGTAG, "Measurement task stopped");
    m_stopped.store(true);
}
