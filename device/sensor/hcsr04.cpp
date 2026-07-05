#include "hcsr04.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <rom/ets_sys.h>
#include <freertos/task.h>

const char*  HCSR04::LOGTAG           = "HCSR04";
const float  HCSR04::INVALID_DISTANCE = -1.0f;

// ---------------------------------------------------------------------------
// HCSR04 — single sensor
// ---------------------------------------------------------------------------

HCSR04::HCSR04(const char *name)
    : m_name(name)
    , m_trigPin(GPIO_NUM_NC)
    , m_echoPin(GPIO_NUM_NC)
    , m_echoStartUs(0)
    , m_echoEndUs(0)
    , m_echoSem(nullptr)
    , m_distanceCm(INVALID_DISTANCE)
    , m_initialized(false)
{}

HCSR04::~HCSR04() {
    shutdown();
}

libesp::ErrorType HCSR04::init(gpio_num_t trigPin, gpio_num_t echoPin) {
    if (m_initialized) {
        ESP_LOGW(LOGTAG, "%s: already initialised", m_name);
        return libesp::ErrorType(ESP_OK);
    }

    m_trigPin = trigPin;
    m_echoPin = echoPin;

    m_echoSem = xSemaphoreCreateBinary();
    if (!m_echoSem) {
        ESP_LOGE(LOGTAG, "%s: failed to create echo semaphore", m_name);
        return libesp::ErrorType(ESP_ERR_NO_MEM);
    }

    // TRIG — output, initially low
    gpio_config_t trigCfg = {};
    trigCfg.pin_bit_mask   = 1ULL << m_trigPin;
    trigCfg.mode           = GPIO_MODE_OUTPUT;
    trigCfg.pull_up_en     = GPIO_PULLUP_DISABLE;
    trigCfg.pull_down_en   = GPIO_PULLDOWN_DISABLE;
    trigCfg.intr_type      = GPIO_INTR_DISABLE;
    esp_err_t err = gpio_config(&trigCfg);
    if (err != ESP_OK) {
        ESP_LOGE(LOGTAG, "%s: TRIG gpio_config failed: %s", m_name, esp_err_to_name(err));
        vSemaphoreDelete(m_echoSem);
        m_echoSem = nullptr;
        return libesp::ErrorType(err);
    }
    gpio_set_level(m_trigPin, 0);

    // ECHO — input with pull-down, interrupt on both edges
    gpio_config_t echoCfg = {};
    echoCfg.pin_bit_mask  = 1ULL << m_echoPin;
    echoCfg.mode          = GPIO_MODE_INPUT;
    echoCfg.pull_up_en    = GPIO_PULLUP_DISABLE;
    echoCfg.pull_down_en  = GPIO_PULLDOWN_ENABLE;
    echoCfg.intr_type     = GPIO_INTR_ANYEDGE;
    err = gpio_config(&echoCfg);
    if (err != ESP_OK) {
        ESP_LOGE(LOGTAG, "%s: ECHO gpio_config failed: %s", m_name, esp_err_to_name(err));
        vSemaphoreDelete(m_echoSem);
        m_echoSem = nullptr;
        return libesp::ErrorType(err);
    }

    // Attach ISR — gpio_install_isr_service() must be called before this
    err = gpio_isr_handler_add(m_echoPin, echoIsrHandler, this);
    if (err != ESP_OK) {
        ESP_LOGE(LOGTAG, "%s: gpio_isr_handler_add failed: %s", m_name, esp_err_to_name(err));
        vSemaphoreDelete(m_echoSem);
        m_echoSem = nullptr;
        return libesp::ErrorType(err);
    }

    m_initialized = true;
    ESP_LOGI(LOGTAG, "%s: init OK — TRIG=GPIO%d  ECHO=GPIO%d",
             m_name, (int)m_trigPin, (int)m_echoPin);
    return libesp::ErrorType(ESP_OK);
}

libesp::ErrorType HCSR04::shutdown() {
    if (!m_initialized) {
        return libesp::ErrorType(ESP_OK);
    }

    gpio_isr_handler_remove(m_echoPin);
    gpio_intr_disable(m_echoPin);

    if (m_echoSem) {
        vSemaphoreDelete(m_echoSem);
        m_echoSem = nullptr;
    }

    m_distanceCm.store(INVALID_DISTANCE, std::memory_order_relaxed);
    m_initialized = false;
    ESP_LOGI(LOGTAG, "%s: shutdown complete", m_name);
    return libesp::ErrorType(ESP_OK);
}

// ISR — runs in interrupt context, keep it minimal
void IRAM_ATTR HCSR04::echoIsrHandler(void* arg) {
    HCSR04* self = static_cast<HCSR04*>(arg);

    if (gpio_get_level(self->m_echoPin) == 1) {
        // Rising edge — ultrasonic burst just sent
        self->m_echoStartUs = esp_timer_get_time();
    } else {
        // Falling edge — echo received
        self->m_echoEndUs = esp_timer_get_time();
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(self->m_echoSem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void HCSR04::measure() {
    if (!m_initialized) {
        return;
    }

    // Drain a stale give left behind by an echo that arrived after a
    // previous measurement timed out, and clear the edge timestamps.
    xSemaphoreTake(m_echoSem, 0);
    m_echoStartUs = 0;
    m_echoEndUs   = 0;

    // 10 µs HIGH pulse on TRIG
    gpio_set_level(m_trigPin, 0);
    ets_delay_us(2);
    gpio_set_level(m_trigPin, 1);
    ets_delay_us(10);
    gpio_set_level(m_trigPin, 0);

    float result = INVALID_DISTANCE;

    // Wait for the falling-edge ISR to signal completion
    if (xSemaphoreTake(m_echoSem, pdMS_TO_TICKS(ECHO_TIMEOUT_MS)) == pdTRUE) {
        // Both timestamps are stable once the semaphore has been given
        int64_t startUs = m_echoStartUs;
        int64_t endUs   = m_echoEndUs;

        if (startUs > 0 && endUs > startUs) {
            // Distance (cm) = round-trip time * speed of sound / 2
            // Speed of sound ≈ 343 m/s = 0.0343 cm/µs
            float distCm = static_cast<float>(endUs - startUs) * 0.0343f / 2.0f;
            if (distCm >= MIN_VALID_CM && distCm <= MAX_VALID_CM) {
                result = distCm;
                ESP_LOGI(LOGTAG, "%s:  %.1f cm  (%lld µs)", m_name, distCm, endUs - startUs);
            } else {
                ESP_LOGD(LOGTAG, "%s: reading out of range (%.1f cm)", m_name, distCm);
            }
        } else {
            // Missed the rising edge — don't fabricate a distance from it
            ESP_LOGW(LOGTAG, "%s: bad echo timing (start=%lld end=%lld)",
                     m_name, startUs, endUs);
        }
    } else {
        // Timeout: object too far away, or sensor not connected
        ESP_LOGD(LOGTAG, "%s: echo timeout — no object in range", m_name);
    }

    m_distanceCm.store(result, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// UltraSonicTask — measures both sensors from one task
// ---------------------------------------------------------------------------

const char* UltraSonicTask::LOGTAG = "UltraSonicTask";

UltraSonicTask::UltraSonicTask()
    : Task("ultrasonic", 4096, 5)
    , m_left("LT")
    , m_right("RT")
    , m_intervalMs(DEFAULT_INTERVAL_MS)
    , m_running(false)
    , m_stopped(true)
    , m_initialized(false)
{}

UltraSonicTask::~UltraSonicTask() {
    shutdown();
}

libesp::ErrorType UltraSonicTask::init(gpio_num_t leftTrig, gpio_num_t leftEcho,
                                       gpio_num_t rightTrig, gpio_num_t rightEcho,
                                       uint32_t measureIntervalMs) {
    if (m_initialized) {
        ESP_LOGW(LOGTAG, "Already initialised");
        return libesp::ErrorType(ESP_OK);
    }

    m_intervalMs = measureIntervalMs;

    libesp::ErrorType et = m_left.init(leftTrig, leftEcho);
    if (!et.ok()) {
        return et;
    }
    et = m_right.init(rightTrig, rightEcho);
    if (!et.ok()) {
        m_left.shutdown();
        return et;
    }

    m_initialized = true;
    ESP_LOGI(LOGTAG, "Init OK — interval=%" PRIu32 " ms", m_intervalMs);
    return libesp::ErrorType(ESP_OK);
}

libesp::ErrorType UltraSonicTask::shutdown() {
    if (!m_initialized) {
        return libesp::ErrorType(ESP_OK);
    }

    // Stop the loop and wait for it to exit before tearing down the sensors,
    // so measure() can't touch a deleted semaphore.
    m_running.store(false);
    while (!m_stopped.load()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    m_left.shutdown();
    m_right.shutdown();

    m_initialized = false;
    ESP_LOGI(LOGTAG, "Shutdown complete");
    return libesp::ErrorType(ESP_OK);
}

void UltraSonicTask::run(void* /*data*/) {
    if (!m_initialized) {
        ESP_LOGE(LOGTAG, "Task started before init() — aborting");
        return;
    }

    m_running.store(true);
    m_stopped.store(false);
    ESP_LOGI(LOGTAG, "Measurement task running");

    while (m_running.load()) {
        m_right.measure();
        // Let the burst/echoes die out so the left sensor doesn't hear them
        vTaskDelay(pdMS_TO_TICKS(INTER_SENSOR_GAP_MS));
        m_left.measure();

        uint32_t remainingMs = (m_intervalMs > INTER_SENSOR_GAP_MS)
                                   ? m_intervalMs - INTER_SENSOR_GAP_MS
                                   : 1;
        vTaskDelay(pdMS_TO_TICKS(remainingMs));
    }

    ESP_LOGI(LOGTAG, "Measurement task stopped");
    m_stopped.store(true);
}
