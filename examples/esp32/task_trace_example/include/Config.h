/** @file Config.h @brief Experimental ESP32 runtime workload settings.
 * @details Compile-time bounds for the dual-core FreeRTOS test adapter.
 * @par Usage Include from the firmware; rebuild after changing any value.
 */
#pragma once
#include <stdint.h>

/** @brief Dedicated binary UART speed; match the Studio connection. */
constexpr uint32_t TRACE_BAUD = 460800u;
/** @brief Worker count; each worker owns an independent trace context. */
constexpr uint32_t WORKER_COUNT = 3u;
/** @brief Application task count: workers, alarm producer, IRQ consumer and
 * the pipeline, low-priority compute and high-priority control scenarios. */
constexpr uint32_t TASK_COUNT = WORKER_COUNT + 5u;
/** @brief ESP-IDF static task stacks, measured in StackType_t entries. */
constexpr uint32_t TASK_STACK_WORDS = 2048u;
/** @brief Alarm producer period in milliseconds. */
constexpr uint32_t ALARM_PERIOD_MS = 50u;
/** @brief Timer alarm delay range in microseconds, shorter than producer period. */
constexpr uint32_t ALARM_MIN_US = 2000u;
/** @brief Upper bound for random one-shot alarm delays in microseconds. */
constexpr uint32_t ALARM_MAX_US = 35000u;
/** @brief Pipeline frame period on core 1; each frame nests four scope levels. */
constexpr uint32_t PIPELINE_PERIOD_MS = 150u;
/** @brief Every Nth pipeline frame decompresses this many times slower. */
constexpr uint32_t PIPELINE_OUTLIER_EVERY = 16u;
/** @brief Slowdown factor of an outlier frame's inflate blocks. */
constexpr uint32_t PIPELINE_OUTLIER_FACTOR = 8u;
/** @brief Low-priority compute period on core 0. */
constexpr uint32_t COMPUTE_PERIOD_MS = 120u;
/** @brief Wall time of one of three compute steps; long enough to be preempted. */
constexpr uint32_t COMPUTE_STEP_US = 10000u;
/** @brief High-priority control period on core 0, preempting the compute task. */
constexpr uint32_t CONTROL_PERIOD_MS = 30u;
