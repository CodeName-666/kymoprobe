/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file main.cpp @brief Experimental dual-core FreeRTOS and real timer ISR trace.
 * @warning Target adapter for ESP32 / Arduino 2.x (ESP-IDF 4.4). SDK drivers
 * allocate during setup. Timer interrupts deliberately do NOT request
 * ESP_INTR_FLAG_IRAM: the portable trace library resides in flash. No flash
 * writes, OTA, WiFi or filesystem operations belong in this test workload.
 */
#include <Arduino.h>
#include <driver/timer.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <KymoCore.h>
#include <kymo_generated_config.h>
#include <kymo_trace_ids.h>
#include <kymo_trace_metadata.h>
#include "Config.h"
#include "Types.h"

static portMUX_TYPE sTraceLock = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t sTaskHandles[TASK_COUNT] = {};
static StaticTask_t sTaskStorage[TASK_COUNT] = {};
static StackType_t sTaskStacks[TASK_COUNT][TASK_STACK_WORDS] = {};
static TaskHandle_t sLoopTask = nullptr;
static uint64_t sSessionId = 0u;
static uint64_t sMetadataTicks = 0u;
static uint8_t sMetadataStage = 0u;
static uint64_t sHealthTicks = 0u;
static bool sIsReady = false;
// Fast/background periods leave UART headroom for the pipeline/compute/control scenarios.
static WorkerConfig_T sWorkers[WORKER_COUNT] = {
    {KYMO_CONTEXT_FAST_WORKER, 50u, 200u, 500u},
    {KYMO_CONTEXT_SLOW_WORKER, 70u, 1500u, 3000u},
    {KYMO_CONTEXT_BACKGROUND_WORKER, 80u, 700u, 1600u}
};
static const uint32_t TASK_CONTEXTS[TASK_COUNT] = {
    KYMO_CONTEXT_FAST_WORKER, KYMO_CONTEXT_SLOW_WORKER,
    KYMO_CONTEXT_BACKGROUND_WORKER, KYMO_CONTEXT_ALARM_PRODUCER,
    KYMO_CONTEXT_IRQ_CONSUMER, KYMO_CONTEXT_PIPELINE,
    KYMO_CONTEXT_COMPUTE_LOW, KYMO_CONTEXT_CONTROL_HIGH
};

/** @brief Shared microsecond clock, callable from task and timer ISR context.
 * @param[in] user Unused callback cookie. @return Monotonic ticks at 1 MHz. */
/*******************************************************************************
 * traceClockTicks
 ******************************************************************************/
extern "C" uint64_t traceClockTicks(void *user)
{
    uint64_t result = static_cast<uint64_t>(esp_timer_get_time());
    (void)user;
    return result;
}

/** @brief Identify the calling CPU. @param[in] user Unused cookie.
 * @return Core index 0 or 1. */
/*******************************************************************************
 * traceCoreId
 ******************************************************************************/
extern "C" uint8_t traceCoreId(void *user)
{
    uint8_t result = static_cast<uint8_t>(xPortGetCoreID());
    (void)user;
    return result;
}

/** @brief Resolve the current or interrupted task; SDK/idle tasks remain unknown.
 * @param[in] user Unused cookie. @return Registered context or zero. */
/*******************************************************************************
 * traceCurrentContext
 ******************************************************************************/
extern "C" uint32_t traceCurrentContext(void *user)
{
    TaskHandle_t current = xTaskGetCurrentTaskHandle();
    uint32_t result = 0u;
    (void)user;
    if (current == sLoopTask) {
        result = KYMO_CONTEXT_MAIN;
    }
    for (uint32_t index = 0u; index < TASK_COUNT; ++index) {
        if ((sTaskHandles[index] != nullptr) && (current == sTaskHandles[index])) {
            result = TASK_CONTEXTS[index];
        }
    }
    return result;
}

/** @brief Serialize recorder access across both CPUs and ISR/task callers.
 * @param[in] user Unused cookie. @note ISR-aware lock; no UART inside it. */
/*******************************************************************************
 * traceCriticalEnter
 ******************************************************************************/
extern "C" void traceCriticalEnter(void *user)
{
    (void)user;
    if (xPortInIsrContext()) {
        portENTER_CRITICAL_ISR(&sTraceLock);
    } else {
        portENTER_CRITICAL(&sTraceLock);
    }
}

/** @brief Release the recorder lock. @param[in] user Unused cookie. */
/*******************************************************************************
 * traceCriticalExit
 ******************************************************************************/
extern "C" void traceCriticalExit(void *user)
{
    (void)user;
    if (xPortInIsrContext()) {
        portEXIT_CRITICAL_ISR(&sTraceLock);
    } else {
        portEXIT_CRITICAL(&sTraceLock);
    }
}

/** @brief Accept only complete UART frames, exclusively from loop().
 * @param[in] user Unused cookie. @param[in] bytes Frame, required.
 * @param[in] length Frame bytes. @return Accepted bytes, or zero for retry. */
/*******************************************************************************
 * traceWriteFrame
 ******************************************************************************/
extern "C" size_t traceWriteFrame(void *user, const uint8_t *bytes, size_t length)
{
    size_t result = 0u;
    (void)user;
    if ((bytes != nullptr) &&
        (length <= static_cast<size_t>(Serial.availableForWrite()))) {
        result = Serial.write(bytes, length);
    }
    return result;
}

/** @brief Supply a boot-specific session. @return Session ID for metadata. */
/*******************************************************************************
 * kymo_trace_session_id
 ******************************************************************************/
extern "C" uint64_t kymo_trace_session_id(void)
{
    return sSessionId;
}

/** @brief Generate task-side random durations. @param[in] minimum Lower bound.
 * @param[in] maximum Upper bound. @return Duration in inclusive range. */
/*******************************************************************************
 * chooseDurationUs
 ******************************************************************************/
static uint32_t chooseDurationUs(uint32_t minimum, uint32_t maximum)
{
    uint32_t result = minimum + (esp_random() % (maximum - minimum + 1u));
    return result;
}

/** @brief Bounded busy work with a nested scope for function-flow inspection.
 * @param[in] context Registered task. @param[in] durationUs Busy interval.
 * @note Wall time includes interruptions; this is not exclusive CPU time. */
/*******************************************************************************
 * runBusyWork
 ******************************************************************************/
static void runBusyWork(uint32_t context, uint32_t durationUs)
{
    uint64_t startTicks = traceClockTicks(nullptr);
    KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, context, KYMO_SYMBOL_BUSY_WORK);
    while ((traceClockTicks(nullptr) - startTicks) < durationUs) {
        __asm__ __volatile__("nop");
    }
}

/** @brief Execute a worker cycle without including its sleep in the scope.
 * @param[in] config Fixed workload parameters. */
/*******************************************************************************
 * runWorkerCycle
 ******************************************************************************/
static void runWorkerCycle(const WorkerConfig_T &config)
{
    uint32_t durationUs = chooseDurationUs(config.minimumWorkUs, config.maximumWorkUs);
    KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, config.contextId, KYMO_SYMBOL_WORKER_CYCLE);
    KYMO_TRACE_MARK(&kymo_generated_trace_recorder, config.contextId,
                    KYMO_SYMBOL_REQUESTED_US, durationUs);
    runBusyWork(config.contextId, durationUs);
}

/** @brief Worker entry, configuration found by registered handle.
 * @param[in] user Unused cookie. @note Blocks on startup gate and periodic delay. */
/*******************************************************************************
 * runWorkerTask
 ******************************************************************************/
static void runWorkerTask(void *user)
{
    uint32_t workerIndex = WORKER_COUNT;
    TaskHandle_t current = xTaskGetCurrentTaskHandle();
    (void)user;
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    for (uint32_t index = 0u; index < WORKER_COUNT; ++index) {
        if (sTaskHandles[index] == current) {
            workerIndex = index;
        }
    }
    while (workerIndex < WORKER_COUNT) {
        runWorkerCycle(sWorkers[workerIndex]);
        vTaskDelay(pdMS_TO_TICKS(sWorkers[workerIndex].periodMs));
    }
    vTaskSuspend(nullptr);
}

/** @brief ISR bounded work: trace pair and task notification only.
 * @param[in] context Registered interrupt. @return Whether consumer woke.
 * @note Registered without IRAM interrupt flag because KymoCore uses flash. */
/*******************************************************************************
 * recordTimerInterrupt
 ******************************************************************************/
static bool IRAM_ATTR recordTimerInterrupt(uint32_t context)
{
    BaseType_t hasWoken = pdFALSE;
    KYMO_TRACE_ISR_ENTER(&kymo_generated_trace_recorder, context);
    vTaskNotifyGiveFromISR(sTaskHandles[4u], &hasWoken);
    KYMO_TRACE_ISR_EXIT(&kymo_generated_trace_recorder, context);
    return hasWoken == pdTRUE;
}

/** @brief First real hardware timer callback. @param[in] user Unused cookie.
 * @return Whether FreeRTOS should yield after the ISR. */
/*******************************************************************************
 * onFastTimer
 ******************************************************************************/
static bool IRAM_ATTR onFastTimer(void *user)
{
    bool result = false;
    (void)user;
    result = recordTimerInterrupt(KYMO_CONTEXT_TIMER_FAST);
    return result;
}

/** @brief Second real hardware timer callback. @param[in] user Unused cookie.
 * @return Whether FreeRTOS should yield after the ISR. */
/*******************************************************************************
 * onSlowTimer
 ******************************************************************************/
static bool IRAM_ATTR onSlowTimer(void *user)
{
    bool result = false;
    (void)user;
    result = recordTimerInterrupt(KYMO_CONTEXT_TIMER_SLOW);
    return result;
}

/** @brief Arm one one-shot hardware alarm relative to its running counter.
 * @param[in] timer Timer index within group 0. @return Success status. */
/*******************************************************************************
 * armRandomAlarm
 ******************************************************************************/
static bool armRandomAlarm(timer_idx_t timer)
{
    uint64_t counterTicks = 0u;
    uint32_t delayUs = chooseDurationUs(ALARM_MIN_US, ALARM_MAX_US);
    bool result = timer_get_counter_value(TIMER_GROUP_0, timer, &counterTicks) == ESP_OK;
    if (result) {
        result = timer_set_alarm_value(TIMER_GROUP_0, timer, counterTicks + delayUs) == ESP_OK;
    }
    if (result) {
        result = timer_set_alarm(TIMER_GROUP_0, timer, TIMER_ALARM_EN) == ESP_OK;
    }
    return result;
}

/** @brief Produce independent random timer alarms, no randomness inside ISRs.
 * @param[in] user Unused cookie. */
/*******************************************************************************
 * runAlarmTask
 ******************************************************************************/
static void runAlarmTask(void *user)
{
    bool isArmed = false;
    (void)user;
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
        {
            KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_ALARM_PRODUCER,
                             KYMO_SYMBOL_ARM_ALARMS);
            isArmed = armRandomAlarm(TIMER_0);
            isArmed = armRandomAlarm(TIMER_1) && isArmed;
            KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_ALARM_PRODUCER,
                            KYMO_SYMBOL_ALARM_OK, isArmed ? 1u : 0u);
        }
        vTaskDelay(pdMS_TO_TICKS(ALARM_PERIOD_MS));
    }
}

/** @brief Handle deferred IRQ work, never blocking within its measured scope.
 * @param[in] user Unused cookie. */
/*******************************************************************************
 * runIrqConsumerTask
 ******************************************************************************/
static void runIrqConsumerTask(void *user)
{
    uint32_t notifications = 0u;
    (void)user;
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
        notifications = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        {
            KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_IRQ_CONSUMER,
                             KYMO_SYMBOL_IRQ_WORK);
            KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_IRQ_CONSUMER,
                            KYMO_SYMBOL_IRQ_COUNT, notifications);
            runBusyWork(KYMO_CONTEXT_IRQ_CONSUMER, 100u);
        }
    }
}

/** @brief Busy-wait without a trace scope; callers own the measured scope.
 * @param[in] durationUs Wall-clock interval, including any preemption. */
/*******************************************************************************
 * spinUs
 ******************************************************************************/
static void spinUs(uint32_t durationUs)
{
    uint64_t startTicks = traceClockTicks(nullptr);
    while ((traceClockTicks(nullptr) - startTicks) < durationUs) {
        __asm__ __volatile__("nop");
    }
}

/** @brief Pipeline level 4: one inflate block, scaled for outlier frames.
 * @param[in] payloadBytes Frame payload size. @param[in] isOutlier Slow frame. */
/*******************************************************************************
 * inflateBlock
 ******************************************************************************/
static void inflateBlock(uint32_t payloadBytes, bool isOutlier)
{
    uint32_t durationUs = 60u + (payloadBytes / 4u);
    KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                     KYMO_SYMBOL_INFLATE_BLOCK);
    spinUs(isOutlier ? durationUs * PIPELINE_OUTLIER_FACTOR : durationUs);
}

/** @brief Pipeline level 3: short checksum, then one to three inflate blocks.
 * @param[in] payloadBytes Frame payload size. @param[in] isOutlier Slow frame. */
/*******************************************************************************
 * parsePayload
 ******************************************************************************/
static void parsePayload(uint32_t payloadBytes, bool isOutlier)
{
    uint32_t blocks = 1u + (payloadBytes / 200u);
    KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                     KYMO_SYMBOL_PARSE_PAYLOAD);
    {
        KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                         KYMO_SYMBOL_CHECKSUM);
        spinUs(20u + (payloadBytes / 16u));
    }
    {
        KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                         KYMO_SYMBOL_DECOMPRESS);
        for (uint32_t index = 0u; index < blocks; ++index) {
            inflateBlock(payloadBytes, isOutlier);
        }
    }
}

/** @brief Pipeline level 1: siblings of clearly different length below one frame.
 * @param[in] frameIndex Running frame number; every Nth frame is an outlier. */
/*******************************************************************************
 * processFrame
 ******************************************************************************/
static void processFrame(uint32_t frameIndex)
{
    uint32_t payloadBytes = chooseDurationUs(64u, 512u);
    bool isOutlier = (frameIndex % PIPELINE_OUTLIER_EVERY) == 0u;
    KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                     KYMO_SYMBOL_PROCESS_FRAME);
    KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                    KYMO_SYMBOL_FRAME_INDEX, frameIndex);
    KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                    KYMO_SYMBOL_PAYLOAD_BYTES, payloadBytes);
    {
        KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                         KYMO_SYMBOL_DECODE_HEADER);
        spinUs(40u);
    }
    parsePayload(payloadBytes, isOutlier);
    {
        KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_PIPELINE,
                         KYMO_SYMBOL_APPLY_RESULT);
        spinUs(150u);
    }
}

/** @brief Core 1 pipeline; timer ISRs on core 1 can land inside its nesting.
 * @param[in] user Unused cookie. */
/*******************************************************************************
 * runPipelineTask
 ******************************************************************************/
static void runPipelineTask(void *user)
{
    uint32_t frameIndex = 0u;
    (void)user;
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
        processFrame(frameIndex);
        ++frameIndex;
        vTaskDelay(pdMS_TO_TICKS(PIPELINE_PERIOD_MS));
    }
}

/** @brief Core 0, low priority: ~30 ms compute that the control task preempts.
 * @param[in] user Unused cookie. @note Wall-time steps include preemption. */
/*******************************************************************************
 * runComputeTask
 ******************************************************************************/
static void runComputeTask(void *user)
{
    uint32_t computeRound = 0u;
    (void)user;
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
        {
            KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_COMPUTE_LOW,
                             KYMO_SYMBOL_LONG_COMPUTE);
            KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_COMPUTE_LOW,
                            KYMO_SYMBOL_COMPUTE_ROUND, computeRound);
            for (uint32_t step = 0u; step < 3u; ++step) {
                KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_COMPUTE_LOW,
                                 KYMO_SYMBOL_MATRIX_STEP);
                spinUs(COMPUTE_STEP_US);
            }
        }
        ++computeRound;
        vTaskDelay(pdMS_TO_TICKS(COMPUTE_PERIOD_MS));
    }
}

/** @brief Core 0, high priority: short nested control cycle every 30 ms.
 * @param[in] user Unused cookie. */
/*******************************************************************************
 * runControlTask
 ******************************************************************************/
static void runControlTask(void *user)
{
    TickType_t wakeTicks = 0u;
    (void)user;
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    wakeTicks = xTaskGetTickCount();
    while (true) {
        {
            KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_CONTROL_HIGH,
                             KYMO_SYMBOL_CONTROL_LOOP);
            spinUs(80u);
            {
                KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_CONTROL_HIGH,
                                 KYMO_SYMBOL_UPDATE_CONTROL);
                spinUs(250u);
            }
        }
        vTaskDelayUntil(&wakeTicks, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
    }
}

/** @brief Initialize paused-alarm timers on setup's core before releasing tasks.
 * @param[in] timer Group 0 timer. @param[in] callback Required ISR callback.
 * @return True if driver initialization and counter startup succeeded. */
/*******************************************************************************
 * initializeTimer
 ******************************************************************************/
static bool initializeTimer(timer_idx_t timer, timer_isr_t callback)
{
    timer_config_t config = {};
    bool result = false;
    config.alarm_en = TIMER_ALARM_DIS;
    config.counter_en = TIMER_PAUSE;
    config.intr_type = TIMER_INTR_LEVEL;
    config.counter_dir = TIMER_COUNT_UP;
    config.auto_reload = TIMER_AUTORELOAD_DIS;
    config.divider = 80u;
    if (callback != nullptr) {
        result = timer_init(TIMER_GROUP_0, timer, &config) == ESP_OK;
    }
    if (result) {
        result = timer_set_counter_value(TIMER_GROUP_0, timer, 0u) == ESP_OK;
    }
    if (result) {
        result = timer_enable_intr(TIMER_GROUP_0, timer) == ESP_OK;
    }
    if (result) {
        result = timer_isr_callback_add(TIMER_GROUP_0, timer, callback, nullptr, 0) == ESP_OK;
    }
    if (result) {
        result = timer_start(TIMER_GROUP_0, timer) == ESP_OK;
    }
    return result;
}

/** @brief Publish metadata immediately, then periodically for late connections. */
/*******************************************************************************
 * queueMetadata
 ******************************************************************************/
static void queueMetadata(void)
{
    uint64_t nowTicks = traceClockTicks(nullptr);
    if ((sMetadataStage == 2u) && ((nowTicks - sMetadataTicks) >= 5000000u)) {
        sMetadataStage = 0u;
    }
    if (kymo_generated_trace_mux.admin_pending == 0u) {
        if ((sMetadataStage == 0u) &&
            (KymoMux_QueueAdmin(&kymo_generated_trace_mux,
                KYMO_TRACE_PAYLOAD_SESSION_META, 1u, kymo_trace_session_metadata,
                kymo_trace_session_metadata_length) == KYMO_OK)) {
            sMetadataStage = 1u;
        } else if ((sMetadataStage == 1u) &&
            (KymoMux_QueueAdmin(&kymo_generated_trace_mux,
                KYMO_TRACE_PAYLOAD_DICTIONARY, 1u, kymo_trace_dictionary,
                kymo_trace_dictionary_length) == KYMO_OK)) {
            sMetadataStage = 2u;
            sMetadataTicks = nowTicks;
        }
    }
}

/** @brief Initialize recorder, static tasks and real timer interrupts.
 * @note Partial initialization leaves tasks gated and alarms disabled. */
/*******************************************************************************
 * setup
 ******************************************************************************/
void setup(void)
{
    bool areTasksReady = true;
    bool areTimersReady = false;
    Serial.begin(TRACE_BAUD);
    sLoopTask = xTaskGetCurrentTaskHandle();
    sSessionId = static_cast<uint64_t>(esp_random()) * 4294967296ULL + esp_random();
    sIsReady = kymo_generated_trace_init(nullptr) != 0u;
    if (sIsReady) {
        for (uint32_t index = 0u; index < WORKER_COUNT; ++index) {
            sTaskHandles[index] = xTaskCreateStaticPinnedToCore(runWorkerTask, "kymo_worker",
                sizeof(sTaskStacks[index]), nullptr, index == 0u ? 3u : 1u,
                sTaskStacks[index], &sTaskStorage[index], index == 1u ? 0 : 1);
            areTasksReady = (sTaskHandles[index] != nullptr) && areTasksReady;
        }
        sTaskHandles[3u] = xTaskCreateStaticPinnedToCore(runAlarmTask, "kymo_alarms",
            sizeof(sTaskStacks[3u]), nullptr, 2u, sTaskStacks[3u], &sTaskStorage[3u], 1);
        sTaskHandles[4u] = xTaskCreateStaticPinnedToCore(runIrqConsumerTask, "kymo_irq_work",
            sizeof(sTaskStacks[4u]), nullptr, 4u, sTaskStacks[4u], &sTaskStorage[4u], 1);
        sTaskHandles[5u] = xTaskCreateStaticPinnedToCore(runPipelineTask, "kymo_pipeline",
            sizeof(sTaskStacks[5u]), nullptr, 2u, sTaskStacks[5u], &sTaskStorage[5u], 1);
        // Same core, different priority: control_high preempts long_compute.
        sTaskHandles[6u] = xTaskCreateStaticPinnedToCore(runComputeTask, "kymo_compute",
            sizeof(sTaskStacks[6u]), nullptr, 1u, sTaskStacks[6u], &sTaskStorage[6u], 0);
        sTaskHandles[7u] = xTaskCreateStaticPinnedToCore(runControlTask, "kymo_control",
            sizeof(sTaskStacks[7u]), nullptr, 5u, sTaskStacks[7u], &sTaskStorage[7u], 0);
        for (uint32_t index = WORKER_COUNT; index < TASK_COUNT; ++index) {
            areTasksReady = areTasksReady && (sTaskHandles[index] != nullptr);
        }
        if (areTasksReady) {
            areTimersReady = initializeTimer(TIMER_0, onFastTimer);
            areTimersReady = initializeTimer(TIMER_1, onSlowTimer) && areTimersReady;
        }
        sIsReady = areTasksReady && areTimersReady;
        if (sIsReady) {
            // Consumer startup notification must precede any possible timer alarm.
            (void)xTaskNotifyGive(sTaskHandles[4u]);
            for (uint32_t index = 0u; index < TASK_COUNT; ++index) {
                if (index != 4u) {
                    (void)xTaskNotifyGive(sTaskHandles[index]);
                }
            }
        }
        KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                        KYMO_SYMBOL_STARTUP_OK, sIsReady ? 1u : 0u);
    }
}

/** @brief Sole UART/mux owner, independently draining both core buffers. */
/*******************************************************************************
 * loop
 ******************************************************************************/
void loop(void)
{
    uint64_t nowTicks = traceClockTicks(nullptr);
    uint32_t drops = 0u;
    uint32_t errors = 0u;
    if (kymo_generated_trace_recorder.initialized != 0u) {
        if ((nowTicks - sHealthTicks) >= 1000000u) {
            traceCriticalEnter(nullptr);
            drops = kymo_generated_trace_recorder.drops[0u] +
                kymo_generated_trace_recorder.drops[1u];
            errors = kymo_generated_trace_recorder.errors[0u] +
                kymo_generated_trace_recorder.errors[1u];
            traceCriticalExit(nullptr);
            KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                            KYMO_SYMBOL_RECORDER_DROPS, drops);
            KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                            KYMO_SYMBOL_RECORDER_ERRORS, errors);
            KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                            KYMO_SYMBOL_STARTUP_OK, sIsReady ? 1u : 0u);
            sHealthTicks = nowTicks;
        }
        queueMetadata();
        kymo_generated_trace_service();
    }
    vTaskDelay(pdMS_TO_TICKS(1u));
}
