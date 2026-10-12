/* Generated static runtime wiring; callbacks remain user-owned. */
#include <kymo_generated_config.h>
#if KYMO_EFFECTIVE_RUNTIME
extern "C" uint32_t kymo_port_clock_ms(void *);
extern "C" uint8_t kymo_port_sample(void *, uint8_t, KymoSample *);
extern "C" uint8_t kymo_port_write(void *, const uint8_t *, uint8_t);
extern "C" uint8_t kymo_port_busy(void *);
extern "C" void kymo_port_service(void *);
#endif

#if KYMO_EFFECTIVE_RUNTIME
static const KymoChannel kymo_channels[] = {

};
static uint32_t kymo_last_sample_ms[0];
const KymoConfig kymo_generated_config = {
    kymo_channels,
    kymo_last_sample_ms,
    kymo_port_clock_ms, nullptr,
    kymo_port_sample, nullptr,
    kymo_port_write, kymo_port_busy, kymo_port_service, nullptr,
    0u
};
 #endif
 
 #if KYMO_EFFECTIVE_TRACE
#include <kymo_trace_metadata.h>
#include <stddef.h>
#include <stdint.h>

extern "C" uint64_t traceClockTicks(void *);
extern "C" uint8_t traceCoreId(void *);
extern "C" uint32_t traceCurrentContext(void *);
extern "C" uint64_t kymo_trace_session_id(void);
extern "C" size_t traceWriteFrame(void *, const uint8_t *, size_t);
extern "C" void traceCriticalEnter(void *);
extern "C" void traceCriticalExit(void *);


KymoTraceRecorder kymo_generated_trace_recorder;
KymoTraceMux kymo_generated_trace_mux;
KymoTraceDataAdapter kymo_generated_trace_data_adapter;
static void *kymo_generated_trace_user;
static uint8_t kymo_generated_metadata_stage;

static size_t kymo_generated_trace_write(void *user, const uint8_t *bytes,
                                         size_t length)
{
    (void)user;
    return traceWriteFrame(kymo_generated_trace_user, bytes, length);
}

static uint8_t kymo_generated_trace_busy(void *user)
{
    uint8_t result = 0u;
    (void)user;
    /* Synchronous writer releases the buffer on return. */
    return result;
}

uint8_t kymo_generated_trace_init(void *user)
{
    uint8_t result = 0u;
    KymoTraceConfig recorder_config = {};
    KymoTraceMuxConfig mux_config = {};
    kymo_generated_trace_user = user;
    kymo_generated_metadata_stage = 0u;
    recorder_config.clock_ticks = traceClockTicks;
    recorder_config.core_id = traceCoreId;
    recorder_config.current_context = traceCurrentContext;
    recorder_config.critical_enter = traceCriticalEnter;
    recorder_config.critical_exit = traceCriticalExit;
    recorder_config.user = user;
    recorder_config.session_id = kymo_trace_session_id();
    recorder_config.clock_domain_id = 1u;
    if ((KymoTraceDataAdapter_Init(&kymo_generated_trace_data_adapter) != 0u) &&
        (KymoTrace_Init(&kymo_generated_trace_recorder, &recorder_config) != 0u)) {
        mux_config.recorder = &kymo_generated_trace_recorder;
        mux_config.write_frame = kymo_generated_trace_write;
        mux_config.is_busy = kymo_generated_trace_busy;
        mux_config.next_data = nullptr;
        mux_config.user = user;
        mux_config.max_frame_size = 128u;
        mux_config.health_period_ticks = 10000u;
        mux_config.max_batch_events = 3u;
        result = KymoTraceMux_Init(&kymo_generated_trace_mux, &mux_config);
    }
    return result;
}

void kymo_generated_trace_service(void)
{
    if ((kymo_generated_trace_mux.initialized != 0u) &&
        (kymo_generated_trace_mux.admin_pending == 0u)) {
        if (kymo_generated_metadata_stage == 0u) {
            if (KymoTraceMux_QueueAdmin(&kymo_generated_trace_mux,
                    KYMO_TRACE_PAYLOAD_SESSION_META, 1u,
                    kymo_trace_session_metadata,
                    kymo_trace_session_metadata_length) != 0u) {
                kymo_generated_metadata_stage = 1u;
            }
        } else if (kymo_generated_metadata_stage == 1u) {
            if (KymoTraceMux_QueueAdmin(&kymo_generated_trace_mux,
                    KYMO_TRACE_PAYLOAD_DICTIONARY, 1u,
                    kymo_trace_dictionary,
                    kymo_trace_dictionary_length) != 0u) {
                kymo_generated_metadata_stage = 2u;
            }
        }
    }
    uint32_t sent_bytes = 0u;
    uint32_t sent_events = 0u;
    for (uint16_t attempt = 0u; attempt < 4u; ++attempt) {
        if ((attempt > 0u) && (sent_bytes >= 256u ||
                              sent_events >= 16u)) {
            break;
        }
        const uint8_t status = KymoTraceMux_Service(&kymo_generated_trace_mux);
        if (status != 1u) {
            break;
        }
        sent_bytes += static_cast<uint32_t>(
            kymo_generated_trace_mux.last_frame_length);
        sent_events += kymo_generated_trace_mux.last_frame_events;
        if (kymo_generated_trace_busy(kymo_generated_trace_mux.config.user) != 0u) {
            break;
        }
    }
}

#endif
