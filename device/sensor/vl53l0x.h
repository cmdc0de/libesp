#pragma once

#include "../../error_type.h"
#include "../../i2cbus.h"
#include "../../i2cdevice.h"
#include "driver/gpio.h"
#include "../../task.h"
#include <stdint.h>
#include <atomic>

// VL53L0X Time-of-Flight distance sensor driver.
//
// Communicates over I2C. Default device address: 0x29.
//
// Pin selection for ESP WROOM 32 on this prototype:
//   SCL → GPIO_NUM_23
//   SDA → GPIO_NUM_22
//   (see pinconfig.h — the sensor lives on the I2C_NUM_1 bus)
//
// Wiring: add 4.7kΩ pull-up resistors on SDA and SCL to 3.3 V.
// Logic level is 3.3 V only — do NOT connect 5 V signals.
//
// Usage (direct, single-shot):
//   VL53L0X tof(PIN_TOF_XSHUNT_RT);
//   tof.init(libesp::I2CBus::get(I2C_NUM_1));
//   VL53L0X::RangingData d;
//   tof.read(d);
//   if (d.isValid()) { /* d.rangeMillimeters */ }
//
// For the badge's front sensor use VL53L0XTask below instead, which owns the
// sensor, measures it 10x/second, and exposes lock-free distance accessors.

class VL53L0X {
public:
    // I2C address (7-bit). Can be changed if multiple sensors share the bus.
    static const uint8_t    DEFAULT_ADDRESS      = 0x29;
    static const uint32_t   DEFAULT_I2C_CLOCK_HZ = 400000;  // 400 kHz fast-mode

    // Result of a single distance measurement.
    struct RangingData {
        RangingData() : rangeMillimeters(0), rangeStatus(0xFF) {}
        uint16_t rangeMillimeters;  // Distance in mm (valid only when isValid())
        uint8_t  rangeStatus;       // 0 = valid; non-zero = error / out-of-range
        bool isValid() const { return rangeStatus == 0; }
    };

public:
    VL53L0X(gpio_num_t xshunt);
    ~VL53L0X();

    // Attach to an already-initialized I2C bus (creates the bus device) and
    // run the full ST init sequence. Call once before any read/startContinuous.
    libesp::ErrorType init(libesp::I2CBus *bus,
                           uint8_t address = DEFAULT_ADDRESS);

    // Block until one measurement completes (single-shot mode).
    // timeoutMs: maximum wait for measurement ready (default 500 ms).
    libesp::ErrorType read(RangingData &data, uint32_t timeoutMs = 1000);

    // Start continuous back-to-back ranging.
    // periodMs = 0  → back-to-back (sensor-paced).
    // periodMs > 0  → timed mode; period must exceed timing budget.
    libesp::ErrorType startContinuous(uint32_t periodMs = 0);

    // Stop continuous ranging and return to single-shot mode.
    libesp::ErrorType stopContinuous();

    // Re-assign the sensor's I2C address (useful for multi-sensor setups).
    // Re-creates the bus device to follow the sensor to its new address.
    libesp::ErrorType setAddress(uint8_t newAddress);

    // Timing budget controls the trade-off between speed and accuracy.
    // Valid range: 20 000 – 2 000 000 µs. Default after init: ~33 000 µs.
    libesp::ErrorType setMeasurementTimingBudget(uint32_t budgetUs);
    uint32_t          getMeasurementTimingBudget() const;

    // Detach from the I2C bus and release the sensor (not the bus itself).
    void shutdown();

protected:
    // Low-level register I/O through the libesp I2C device.
    libesp::ErrorType readReg(uint8_t reg, uint8_t &value);
    libesp::ErrorType writeReg(uint8_t reg, uint8_t value);
    libesp::ErrorType readReg16(uint8_t reg, uint16_t &value);
    libesp::ErrorType writeReg16(uint8_t reg, uint16_t value);
    libesp::ErrorType readMulti(uint8_t reg, uint8_t *buf, uint8_t len);
    libesp::ErrorType writeMulti(uint8_t reg, const uint8_t *buf, uint8_t len);

    // Initialization helpers (called from init()).
    libesp::ErrorType loadTuningSettings();
    libesp::ErrorType getSpadInfo(uint8_t &count, bool &isAperture);
    libesp::ErrorType performSingleRefCalibration(uint8_t vhvInitByte);

    // Timing-budget helpers.
    libesp::ErrorType getSequenceStepEnables(bool &tcc, bool &dss,
                                             bool &msrc, bool &preRange,
                                             bool &finalRange);
    libesp::ErrorType getSequenceStepTimeouts(bool preRangeEnabled,
                                              uint16_t &preRangeVcselPeriodPclks,
                                              uint32_t &msrcDssTccUs,
                                              uint32_t &preRangeUs,
                                              uint32_t &finalRangeUs,
                                              uint16_t &finalRangeVcselPeriodPclks);
    uint32_t timeoutMclksToMicroseconds(uint16_t mclks, uint8_t vcselPeriodPclks) const;
    uint32_t timeoutMicrosecondsToMclks(uint32_t us,    uint8_t vcselPeriodPclks) const;
    uint16_t decodeTimeout(uint16_t regVal) const;
    uint16_t encodeTimeout(uint32_t mclks)  const;

private:
    libesp::I2CBus    *Bus;
    libesp::I2CDevice *Device;
    uint8_t  Address;
    uint8_t  StopVariable;              // Stored during init, used in ranging cmds
    uint32_t MeasurementTimingBudgetUs;
    gpio_num_t xShunt;
    bool     ContinuousMode;
    bool     Initialized;

    static const char *LOGTAG;
};

// Owns the badge's single front-facing VL53L0X and measures it from its own
// FreeRTOS task, 10 times per second. Distance is published through an atomic
// so the accessors never lock — safe to call from any task.
//
// Usage:
//   VL53L0XTask tof(PIN_TOF_XSHUNT_RT);
//   tof.init(libesp::I2CBus::get(I2C_NUM_1));
//   tof.start();
//   float cm = tof.getDistanceCm();   // non-blocking, INVALID_DISTANCE if none
//   tof.shutdown();
class VL53L0XTask : public Task {
public:
    static constexpr float   INVALID_DISTANCE    = -1.0f;
    static const int32_t     INVALID_DISTANCE_MM = -1;
    static const uint32_t    DEFAULT_INTERVAL_MS = 100;  // 10 measurements/second
    static const uint32_t    READ_TIMEOUT_MS     = 80;   // > ~33ms timing budget

    explicit VL53L0XTask(gpio_num_t xshunt);
    ~VL53L0XTask();

    libesp::ErrorType init(libesp::I2CBus *bus,
                           uint8_t address = VL53L0X::DEFAULT_ADDRESS,
                           uint32_t measureIntervalMs = DEFAULT_INTERVAL_MS);

    // Stop the measurement loop, wait for it to exit, then release the sensor.
    libesp::ErrorType shutdown();

    // Lock-free: latest distance in mm, or INVALID_DISTANCE_MM when the last
    // measurement failed / was out of range.
    int32_t getDistanceMm() const {
        return m_distanceMm.load(std::memory_order_relaxed);
    }

    // Lock-free: latest distance in cm, or INVALID_DISTANCE.
    float getDistanceCm() const {
        int32_t mm = getDistanceMm();
        return (mm < 0) ? INVALID_DISTANCE : static_cast<float>(mm) / 10.0f;
    }

    // Lock-free: true if the last measurement produced a valid reading.
    bool isDataValid() const {
        return getDistanceMm() >= 0;
    }

protected:
    // FreeRTOS task body — do not call directly.
    void run(void* data) override;

private:
    VL53L0X  m_sensor;
    uint32_t m_intervalMs;

    std::atomic<int32_t> m_distanceMm;
    std::atomic<bool>    m_running;
    std::atomic<bool>    m_stopped;
    bool m_initialized;

    static const char *LOGTAG;
};
