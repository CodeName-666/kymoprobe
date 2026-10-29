/* SPDX-License-Identifier: MIT */
#ifndef KYMO_FREERTOS_TRACE_ADAPTER_H
#define KYMO_FREERTOS_TRACE_ADAPTER_H

#include <stdint.h>
#include <KymoCore.h>

/** @brief Fixed storage for explicit FreeRTOS trace hook integration.
 * @details Task handles are opaque tokens and are never dereferenced. IDs are
 * assigned monotonically and are not reused within the session. Exhaustion
 * leaves later tasks untracked. Every call must be serialized by the supplied
 * paired lock callbacks, including calls from scheduler hook context.
 */
typedef struct KymoFreeRtosTraceAdapter {
    KymoTraceRecorder *recorder; /**< Recorder receiving task events. */
    void (*lock)(void *user); /**< Bounded lock for adapter state. */
    void (*unlock)(void *user); /**< Paired unlock for adapter state. */
    uint32_t (*context_for_task)(void *user, const void *task_handle); /**< Optional configured-ID resolver. */
    void *user; /**< Opaque callback context. */
    const void *handles[KYMO_TRACE_MAX_CONTEXTS]; /**< Opaque task handles. */
    uint32_t context_ids[KYMO_TRACE_MAX_CONTEXTS]; /**< Assigned context IDs. */
    uint32_t generations[KYMO_TRACE_MAX_CONTEXTS]; /**< Per-slot task generation. */
    uint8_t active[KYMO_TRACE_MAX_CONTEXTS]; /**< Nonzero for live mapping. */
    const void *outgoing[KYMO_TRACE_CORE_COUNT]; /**< Outgoing task per core. */
    uint32_t next_context_id; /**< Monotonic unnamed fallback ID. */
} KymoFreeRtosTraceAdapter;

/** @brief Initialize hook adapter state for one recorder session.
 * @param[out] adapter Caller-owned fixed adapter storage.
 * @param[in,out] recorder Initialized KymoTrace recorder.
 * @param[in] lock Bounded mapping-table lock callback.
 * @param[in] unlock Paired mapping-table unlock callback.
 * @param[in] user Opaque value passed to the mapping lock callbacks.
 * @return 1 when initialized; 0 for invalid arguments.
 */
uint8_t KymoFreeRtosTrace_Init(KymoFreeRtosTraceAdapter *adapter,
    KymoTraceRecorder *recorder, void (*lock)(void *),
    void (*unlock)(void *), void *user);
/** @brief Install an optional resolver for IDs declared in the dictionary.
 * @details Return zero to request a monotonic fallback ID. Otherwise return a
 * configured context ID in 1..4294967294. Resolvers must be bounded,
 * allocation-free and safe in the hook's calling context. Install before the
 * kernel starts invoking hooks.
 * @param[in,out] adapter Initialized adapter.
 * @param[in] context_for_task Resolver callback, or null to use fallback IDs.
 */
void KymoFreeRtosTrace_SetContextResolver(KymoFreeRtosTraceAdapter *adapter,
    uint32_t (*context_for_task)(void *user, const void *task_handle));
/** @brief Record a task creation hook; handles must be stable opaque tokens.
 * @param[in,out] adapter Initialized adapter.
 * @param[in] task_handle Opaque handle supplied by the kernel.
 * @return Assigned context ID, or zero when capacity/IDs are exhausted.
 */
uint32_t KymoFreeRtosTrace_TaskCreate(KymoFreeRtosTraceAdapter *adapter,
    const void *task_handle);
/** @brief Record task deletion and retain its mapping for ID non-reuse.
 * @param[in,out] adapter Initialized adapter.
 * @param[in] task_handle Opaque handle supplied by the kernel.
 */
void KymoFreeRtosTrace_TaskDelete(KymoFreeRtosTraceAdapter *adapter,
    const void *task_handle);
/** @brief Capture the outgoing task before the kernel switches context.
 * @param[in,out] adapter Initialized adapter.
 * @param[in] core_id Core index in the configured recorder range.
 * @param[in] task_handle Outgoing opaque task handle, or null when unknown.
 */
void KymoFreeRtosTrace_SwitchedOut(KymoFreeRtosTraceAdapter *adapter,
    uint8_t core_id, const void *task_handle);
/** @brief Emit an outgoing/incoming scheduler transition after the switch.
 * @param[in,out] adapter Initialized adapter.
 * @param[in] core_id Core index in the configured recorder range.
 * @param[in] task_handle Incoming opaque task handle, or null when unknown.
 */
void KymoFreeRtosTrace_SwitchedIn(KymoFreeRtosTraceAdapter *adapter,
    uint8_t core_id, const void *task_handle);

#endif
