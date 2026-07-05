#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include "../../task.h"
#include "../../error_type.h"
#include <atomic>

// HC-SR04 Ultrasonic Distance Sensor Driver
//
// Protocol:
//   TRIG: Send a 10µs HIGH pulse to initiate a measurement.
//   ECHO: Goes HIGH when the ultrasonic burst is sent, LOW when the echo
//         returns. Pulse width maps to round-trip travel time.
//   Distance (cm) = echo_duration_us * 0.0343 / 2
//   Valid range: ~2 cm – 400 cm
//
// HCSR04 drives a single sensor and owns no task. measure() performs one
// blocking measurement (bounded by ECHO_TIMEOUT_MS) and must run in a task
// context — see UltraSonicTask below, which owns the left/right sensors and
// paces the measurements. getDistanceCm()/isDataValid() are lock-free and
// safe to call from any task.
//
// Notes:
//   - gpio_install_isr_service() must be called before init() (done in app_main).
//   - Pins must not be shared with other peripherals.

class HCSR04 {
public:
    static const float    INVALID_DISTANCE;
    static const uint32_t ECHO_TIMEOUT_MS = 30;   // ~5 m max range @ 343 m/s
    static constexpr float MIN_VALID_CM   = 2.0f;
    static constexpr float MAX_VALID_CM   = 400.0f;

    // name is used only for logging (e.g. "LT", "RT")
    explicit HCSR04(const char *name);
    ~HCSR04();

    // Configure GPIO pins and attach the echo ISR.
    // gpio_install_isr_service() must already have been called.
    libesp::ErrorType init(gpio_num_t trigPin, gpio_num_t echoPin);

    // Trigger one measurement and block (≤ ECHO_TIMEOUT_MS) for the echo,
    // then update the stored distance. Task context only; not reentrant.
    void measure();

    // Release GPIO/semaphore resources.
    // Only call once no task can be inside measure().
    libesp::ErrorType shutdown();

    // Lock-free: most recent distance in cm, or INVALID_DISTANCE when the
    // last measurement timed out or was out of the sensor's valid range.
    float getDistanceCm() const {
        return m_distanceCm.load(std::memory_order_relaxed);
    }

    // Lock-free: true if the last measurement produced an in-range reading.
    bool isDataValid() const {
        return getDistanceCm() != INVALID_DISTANCE;
    }

private:
    static void IRAM_ATTR echoIsrHandler(void* arg);

    const char *m_name;
    gpio_num_t  m_trigPin;
    gpio_num_t  m_echoPin;

    // Written from ISR, read from the measuring task after taking m_echoSem
    // (the semaphore give/take orders the accesses).
    volatile int64_t m_echoStartUs;
    volatile int64_t m_echoEndUs;

    SemaphoreHandle_t m_echoSem;      // signals falling edge from ISR → task

    std::atomic<float> m_distanceCm;
    bool m_initialized;

    static const char* LOGTAG;
};

// Owns both badge ultrasonic sensors and measures them sequentially from a
// single FreeRTOS task (simultaneous triggering lets each sensor hear the
// other's burst). Each sensor is measured once per interval.
//
// Usage:
//   UltraSonicTask sonar;
//   sonar.init(TRIG_LT, ECHO_LT, TRIG_RT, ECHO_RT, 1000);
//   sonar.start();
//   float cm = sonar.getLeftDistanceCm();   // non-blocking
//   sonar.shutdown();
class UltraSonicTask : public Task {
public:
    static const uint32_t DEFAULT_INTERVAL_MS = 100;  // ms between measurement rounds
    static const uint32_t INTER_SENSOR_GAP_MS = 60;   // let echoes die out between sensors

    UltraSonicTask();
    ~UltraSonicTask();

    libesp::ErrorType init(gpio_num_t leftTrig, gpio_num_t leftEcho,
                           gpio_num_t rightTrig, gpio_num_t rightEcho,
                           uint32_t measureIntervalMs = DEFAULT_INTERVAL_MS);

    // Stop the measurement loop, wait for it to exit, then release the sensors.
    libesp::ErrorType shutdown();

    // Non-blocking, safe from any task.
    float getLeftDistanceCm() const  { return m_left.getDistanceCm(); }
    float getRightDistanceCm() const { return m_right.getDistanceCm(); }
    bool  isLeftValid() const        { return m_left.isDataValid(); }
    bool  isRightValid() const       { return m_right.isDataValid(); }

protected:
    // FreeRTOS task body — do not call directly.
    void run(void* data) override;

private:
    HCSR04   m_left;
    HCSR04   m_right;
    uint32_t m_intervalMs;

    std::atomic<bool> m_running;
    std::atomic<bool> m_stopped;
    bool m_initialized;

    static const char* LOGTAG;
};
