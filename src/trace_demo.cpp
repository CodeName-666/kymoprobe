/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file main.cpp @brief ESP32 UART runtime trace example. */
#include "demo_selection.h"
#include <Arduino.h>
#include <KymoCore.h>

#if ((KYMO_DEMO_MODE == KYMO_DEMO_TRACE) || (KYMO_DEMO_MODE == KYMO_DEMO_TRACE_FLOW)) && KYMO_EFFECTIVE_TRACE
#include <esp_random.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/task.h>
#include <kymo_generated_config.h>
#include <kymo_trace_ids.h>
#include <kymo_trace_metadata.h>

/** Pause per loop in ms. A loop records five events, sent as two batch frames
 *  (~340 bytes); 100 loops/s (~34 KB/s) fit UART 460800 (~46 KB/s). Without
 *  it the loop outruns the link and the recorder drops most records. */
#ifndef TRACE_DEMO_LOOP_DELAY_MS
#define TRACE_DEMO_LOOP_DELAY_MS 10u
#endif

/** Serial TX buffer in bytes. The default (UART FIFO, ~128 bytes) holds less
 *  than one batch frame, so the trace service could send only one frame per
 *  loop and the ring overflowed. */
#ifndef TRACE_DEMO_TX_BUFFER_BYTES
#define TRACE_DEMO_TX_BUFFER_BYTES 1024u
#endif

portMUX_TYPE traceMuxLock = portMUX_INITIALIZER_UNLOCKED;
uint64_t traceSessionId = 0u;
uint32_t loopCounter = 0u;
uint8_t metadataStage = 0u;
uint64_t metadataCycleStartTicks = 0u;

/*******************************************************************************
 * traceClockTicks
 ******************************************************************************/
extern "C" uint64_t traceClockTicks(void *user)
{
    uint64_t result = static_cast<uint64_t>(esp_timer_get_time());
    (void)user;
    return result;
}

/*******************************************************************************
 * traceCoreId
 ******************************************************************************/
extern "C" uint8_t traceCoreId(void *user)
{
    uint8_t result = static_cast<uint8_t>(xPortGetCoreID());
    (void)user;
    return result;
}

/*******************************************************************************
 * traceCurrentContext
 ******************************************************************************/
extern "C" uint32_t traceCurrentContext(void *user)
{
    uint32_t result = KYMO_CONTEXT_MAIN;
    (void)user;
    return result;
}

/*******************************************************************************
 * traceCriticalEnter
 ******************************************************************************/
extern "C" void traceCriticalEnter(void *user)
{
    (void)user;
    portENTER_CRITICAL(&traceMuxLock);
}

/*******************************************************************************
 * traceCriticalExit
 ******************************************************************************/
extern "C" void traceCriticalExit(void *user)
{
    (void)user;
    portEXIT_CRITICAL(&traceMuxLock);
}

/*******************************************************************************
 * traceWriteFrame
 ******************************************************************************/
extern "C" size_t traceWriteFrame(void *user, const uint8_t *bytes,
                                  size_t length)
{
    size_t result = 0u;
    (void)user;
    if ((bytes != nullptr) && (Serial.availableForWrite() >=
                               static_cast<int>(length))) {
        result = Serial.write(bytes, length);
    }
    return result;
}

/*******************************************************************************
 * traceIsBusy
 ******************************************************************************/
extern "C" uint8_t traceIsBusy(void *user)
{
    uint8_t result = Serial.availableForWrite() <= 0 ? 1u : 0u;
    (void)user;
    return result;
}

/*******************************************************************************
 * traceInstrumentedWork
 ******************************************************************************/
void traceInstrumentedWork(uint32_t iteration)
{
    KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                     KYMO_SYMBOL_SIMULATED_WORK);
    KYMO_TRACE_MARK(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                    KYMO_SYMBOL_ITERATION, iteration);
#if KYMO_EFFECTIVE_TRACE_FLOW
    const uint32_t correlation = iteration + 1u;
    KYMO_TRACE_FLOW_SEND(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                        KYMO_SYMBOL_FLOW_PRODUCER, correlation);
#endif
    for (volatile uint32_t step = 0u; step < 300u; ++step) {
        (void)step;
    }
#if KYMO_EFFECTIVE_TRACE_FLOW
    KYMO_TRACE_FLOW_RECEIVE(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                           KYMO_SYMBOL_FLOW_CONSUMER, correlation);
#endif
}

/*******************************************************************************
 * traceQueueMetadata
 ******************************************************************************/
void traceQueueMetadata(void)
{
    KymoStatus queued = KYMO_REJECTED;
    if ((metadataStage == 0u) &&
        ((traceClockTicks(nullptr) - metadataCycleStartTicks) >= 5000000u)) {
        if (kymo_generated_trace_mux.admin_pending == 0u) {
            queued = KymoMux_QueueAdmin(&kymo_generated_trace_mux,
                KYMO_TRACE_PAYLOAD_SESSION_META,
                1u, kymo_trace_session_metadata,
                kymo_trace_session_metadata_length);
            if (queued == KYMO_OK) {
                metadataStage = 1u;
            }
        }
    } else if ((metadataStage == 1u) &&
               (kymo_generated_trace_mux.admin_pending == 0u)) {
        queued = KymoMux_QueueAdmin(&kymo_generated_trace_mux,
            KYMO_TRACE_PAYLOAD_DICTIONARY,
            1u, kymo_trace_dictionary,
            kymo_trace_dictionary_length);
        if (queued == KYMO_OK) {
            metadataStage = 2u;
        }
    } else if ((metadataStage == 2u) &&
               (kymo_generated_trace_mux.admin_pending == 0u)) {
        metadataStage = 3u;
        metadataCycleStartTicks = traceClockTicks(nullptr);
    } else if ((metadataStage == 3u) &&
               ((traceClockTicks(nullptr) - metadataCycleStartTicks) >=
                5000000u)) {
        metadataStage = 0u;
    }
}
/*******************************************************************************
 * kymo_trace_session_id
 ******************************************************************************/
extern "C" uint64_t kymo_trace_session_id(void)
{
    return traceSessionId;
}

/*******************************************************************************
 * demoTraceInit
 ******************************************************************************/
void demoTraceInit(void)
{
    (void)Serial.setTxBufferSize(TRACE_DEMO_TX_BUFFER_BYTES); /* before begin */
    Serial.begin(460800u);
    traceSessionId = (static_cast<uint64_t>(esp_random()) << 32u) |
        static_cast<uint64_t>(esp_random());
    (void)kymo_generated_trace_init(nullptr);
    metadataStage = 0u;
    metadataCycleStartTicks = traceClockTicks(nullptr);
}

/*******************************************************************************
 * demoTraceMain
 ******************************************************************************/
void demoTraceMain(void)
{
    delay(TRACE_DEMO_LOOP_DELAY_MS); /* outside the scope: not loop run time */
    KYMO_TRACE_SCOPE(&kymo_generated_trace_recorder, KYMO_CONTEXT_MAIN,
                     KYMO_SYMBOL_MAIN_LOOP);
    traceInstrumentedWork(loopCounter++);
    kymo_generated_trace_service();
    traceQueueMetadata();
    yield();
}

#endif /* selected trace application */
