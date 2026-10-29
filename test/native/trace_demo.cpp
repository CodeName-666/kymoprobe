/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file trace_demo.cpp @brief Host-run KymoTrace end-to-end source. */
#include <chrono>
#include <cstdint>
#include <cstdio>
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif
#include <KymoCore.h>
#include <kymo_trace_ids.h>
#include <kymo_trace_metadata.h>

namespace {
constexpr uint16_t kClockDomain = 1u;
constexpr uint16_t kMaxFrameBytes = 128u;
KymoTraceRecorder traceRecorder;
KymoMux transportMux;
uint64_t traceSessionId = 0u;
uint8_t metadataStage = 0u;
uint64_t metadataCycleStartTicks = 0u;

/*******************************************************************************
 * kymo_trace_session_id
 ******************************************************************************/
extern "C" uint64_t kymo_trace_session_id(void)
{
    return traceSessionId;
}

/*******************************************************************************
 * configureBinaryOutput
 ******************************************************************************/
void configureBinaryOutput(void)
{
#if defined(_WIN32)
    (void)_setmode(_fileno(stdout), _O_BINARY);
#endif
}

/*******************************************************************************
 * traceClockTicks
 ******************************************************************************/
uint64_t traceClockTicks(void *user)
{
    const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t result = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
    (void)user;
    return result;
}

/*******************************************************************************
 * traceCoreId
 ******************************************************************************/
uint8_t traceCoreId(void *user)
{
    uint8_t result = 0u;
    (void)user;
    return result;
}

/*******************************************************************************
 * traceCurrentContext
 ******************************************************************************/
uint32_t traceCurrentContext(void *user)
{
    uint32_t result = KYMO_CONTEXT_MAIN;
    (void)user;
    return result;
}

/*******************************************************************************
 * traceWriteFrame
 ******************************************************************************/
size_t traceWriteFrame(void *user, const uint8_t *bytes, size_t length)
{
    size_t result = 0u;
    (void)user;
    if (bytes != nullptr) {
        result = std::fwrite(bytes, 1u, length, stdout);
    }
    return result;
}

/*******************************************************************************
 * traceIsBusy
 ******************************************************************************/
uint8_t traceIsBusy(void *user)
{
    uint8_t result = 0u;
    (void)user;
    return result;
}

/*******************************************************************************
 * traceInstrumentedWork
 ******************************************************************************/
void traceInstrumentedWork(uint32_t iteration)
{
    KYMO_TRACE_SCOPE(&traceRecorder, KYMO_CONTEXT_MAIN,
                     KYMO_SYMBOL_SIMULATED_WORK);
    KYMO_TRACE_MARK(&traceRecorder, KYMO_CONTEXT_MAIN,
                    KYMO_SYMBOL_ITERATION, iteration);
}

/*******************************************************************************
 * traceQueueMetadata
 ******************************************************************************/
void traceQueueMetadata(void)
{
    KymoStatus queued = KYMO_REJECTED;
    if (metadataStage == 0u) {
        queued = KymoMux_QueueAdmin(&transportMux,
            KYMO_TRACE_PAYLOAD_SESSION_META, 1u,
            kymo_trace_session_metadata,
            kymo_trace_session_metadata_length);
        if (queued == KYMO_OK) {
            metadataStage = 1u;
        }
    } else if ((metadataStage == 1u) &&
               (transportMux.admin_pending == 0u)) {
        queued = KymoMux_QueueAdmin(&transportMux,
            KYMO_TRACE_PAYLOAD_DICTIONARY, 1u,
            kymo_trace_dictionary,
            kymo_trace_dictionary_length);
        if (queued == KYMO_OK) {
            metadataStage = 2u;
        }
    } else if ((metadataStage == 2u) &&
               (transportMux.admin_pending == 0u)) {
        metadataStage = 3u;
        metadataCycleStartTicks = traceClockTicks(nullptr);
    } else if ((metadataStage == 3u) &&
               ((traceClockTicks(nullptr) - metadataCycleStartTicks) >=
                5000000u)) {
        metadataStage = 0u;
    }
}
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    KymoTraceConfig recorderConfig = {};
    KymoMuxConfig muxConfig = {};
    KymoStatus result = KYMO_REJECTED;
    configureBinaryOutput();
    traceSessionId = traceClockTicks(nullptr);
    recorderConfig.clock_ticks = traceClockTicks;
    recorderConfig.core_id = traceCoreId;
    recorderConfig.current_context = traceCurrentContext;
    recorderConfig.session_id = kymo_trace_session_id();
    recorderConfig.clock_domain_id = kClockDomain;
    result = KymoTrace_Init(&traceRecorder, &recorderConfig);
    muxConfig.recorder = &traceRecorder;
    muxConfig.write = traceWriteFrame;
    muxConfig.busy = traceIsBusy;
    muxConfig.max_frame_size = kMaxFrameBytes;
    muxConfig.health_period_ticks = 0u;
    result = (result == KYMO_OK) ? KymoMux_Init(&transportMux, &muxConfig) : result;
    if (result == KYMO_OK) {
        for (uint32_t iteration = 0u; iteration < 128u; ++iteration) {
            KYMO_TRACE_SCOPE(&traceRecorder, KYMO_CONTEXT_MAIN,
                             KYMO_SYMBOL_MAIN_LOOP);
            traceInstrumentedWork(iteration);
            for (uint8_t servicePass = 0u; servicePass < 8u; ++servicePass) {
                (void)KymoMux_Service(&transportMux);
            }
            traceQueueMetadata();
        }
        for (uint16_t servicePass = 0u; servicePass < 512u; ++servicePass) {
            (void)KymoMux_Service(&transportMux);
            traceQueueMetadata();
        }
    }
    return (result == KYMO_OK) ? 0 : 1;
}
