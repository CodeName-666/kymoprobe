/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial */
/*
 * Source-built FreeRTOS hook adapter. Include the macro bindings in the
 * FreeRTOSConfig.h used to compile the kernel; do not install these macros
 * over an existing application hook without chaining that hook explicitly.
 */
#include "kymo_freertos_trace_adapter.h"

#if KYMO_EFFECTIVE_TRACE_SCHEDULER
namespace {

/*******************************************************************************
 * findTask
 ******************************************************************************/
int findTask(const KymoFreeRtosTraceAdapter *adapter, const void *handle)
{
    int result = -1;
    for (uint16_t slot = 0u; slot < KYMO_TRACE_MAX_CONTEXTS; ++slot) {
        if ((adapter->handles[slot] == handle) && (handle != nullptr) &&
            (adapter->active[slot] != 0u)) {
            result = static_cast<int>(slot);
            break;
        }
    }
    return result;
}

/*******************************************************************************
 * contextFor
 ******************************************************************************/
uint32_t contextFor(const KymoFreeRtosTraceAdapter *adapter,
                    const void *handle)
{
    uint32_t result = 0u;
    int slot = findTask(adapter, handle);
    if (slot >= 0) {
        result = adapter->context_ids[static_cast<uint16_t>(slot)];
    }
    return result;
}
}

/*******************************************************************************
 * KymoFreeRtosTrace_Init
 ******************************************************************************/
uint8_t KymoFreeRtosTrace_Init(KymoFreeRtosTraceAdapter *adapter,
    KymoTraceRecorder *recorder, void (*lock)(void *),
    void (*unlock)(void *), void *user)
{
    uint8_t result = 0u;
    if ((adapter != nullptr) && (recorder != nullptr) &&
        (lock != nullptr) && (unlock != nullptr)) {
        adapter->recorder = recorder;
        adapter->lock = lock;
        adapter->unlock = unlock;
        adapter->user = user;
        /* Reserve the high half of the ID space for unnamed fallback tasks. */
        adapter->next_context_id = 0x80000000u;
        adapter->context_for_task = nullptr;
        for (uint16_t slot = 0u; slot < KYMO_TRACE_MAX_CONTEXTS; ++slot) {
            adapter->handles[slot] = nullptr;
            adapter->context_ids[slot] = 0u;
            adapter->generations[slot] = 0u;
            adapter->active[slot] = 0u;
        }
        for (uint8_t core = 0u; core < KYMO_TRACE_CORE_COUNT; ++core) {
            adapter->outgoing[core] = nullptr;
        }
        result = 1u;
    }
    return result;
}

/*******************************************************************************
 * KymoFreeRtosTrace_SetContextResolver
 ******************************************************************************/
void KymoFreeRtosTrace_SetContextResolver(KymoFreeRtosTraceAdapter *adapter,
    uint32_t (*context_for_task)(void *user, const void *task_handle))
{
    if (adapter != nullptr) {
        adapter->lock(adapter->user);
        adapter->context_for_task = context_for_task;
        adapter->unlock(adapter->user);
    }
}

/*******************************************************************************
 * KymoFreeRtosTrace_TaskCreate
 ******************************************************************************/
uint32_t KymoFreeRtosTrace_TaskCreate(KymoFreeRtosTraceAdapter *adapter,
    const void *task_handle)
{
    uint32_t result = 0u;
    uint32_t createdGeneration = 0u;
    uint8_t created = 0u;
    if ((adapter != nullptr) && (task_handle != nullptr)) {
        uint32_t resolved = 0u;
        if (adapter->context_for_task != nullptr) {
            resolved = adapter->context_for_task(adapter->user, task_handle);
        }
        if (resolved == UINT32_MAX) {
            resolved = 0u;
        }
        adapter->lock(adapter->user);
        int slot = findTask(adapter, task_handle);
        if (slot < 0) {
            if (resolved != 0u) {
                for (uint16_t candidate = 0u;
                     (candidate < KYMO_TRACE_MAX_CONTEXTS) && (slot < 0);
                     ++candidate) {
                    if ((adapter->active[candidate] == 0u) &&
                        (adapter->context_ids[candidate] == resolved)) {
                        slot = static_cast<int>(candidate);
                    }
                }
            }
            for (uint16_t candidate = 0u;
                 (candidate < KYMO_TRACE_MAX_CONTEXTS) && (slot < 0);
                 ++candidate) {
                if ((adapter->active[candidate] == 0u) &&
                    ((adapter->context_ids[candidate] == 0u) ||
                     (adapter->context_ids[candidate] >= 0x80000000u))) {
                    slot = static_cast<int>(candidate);
                }
            }
            if ((slot >= 0) && (adapter->next_context_id != 0u)) {
                uint16_t index = static_cast<uint16_t>(slot);
                uint32_t context = resolved;
                if (context == 0u) {
                    if (adapter->next_context_id < UINT32_MAX) {
                        context = adapter->next_context_id++;
                    }
                }
                for (uint16_t activeSlot = 0u;
                     activeSlot < KYMO_TRACE_MAX_CONTEXTS; ++activeSlot) {
                    if ((adapter->active[activeSlot] != 0u) &&
                        (adapter->context_ids[activeSlot] == context)) {
                        context = 0u;
                    }
                }
                if (context != 0u) {
                    uint32_t generation = 1u;
                    if ((resolved != 0u) &&
                        (adapter->context_ids[index] == resolved)) {
                        if (adapter->generations[index] != UINT32_MAX) {
                            generation = adapter->generations[index] + 1u;
                        } else {
                            context = 0u;
                        }
                    }
                    if (context != 0u) {
                        adapter->handles[index] = task_handle;
                        adapter->context_ids[index] = context;
                        adapter->generations[index] = generation;
                        adapter->active[index] = 1u;
                        created = 1u;
                        createdGeneration = generation;
                        result = context;
                    }
                }
            }
        } else {
            result = adapter->context_ids[static_cast<uint16_t>(slot)];
        }
        adapter->unlock(adapter->user);
        if (created != 0u) {
            KymoTrace_TaskCreate(adapter->recorder, result, createdGeneration);
        }
    }
    return result;
}

/*******************************************************************************
 * KymoFreeRtosTrace_TaskDelete
 ******************************************************************************/
void KymoFreeRtosTrace_TaskDelete(KymoFreeRtosTraceAdapter *adapter,
    const void *task_handle)
{
    uint32_t context = 0u;
    uint32_t generation = 0u;
    if ((adapter != nullptr) && (task_handle != nullptr)) {
        adapter->lock(adapter->user);
        int slot = findTask(adapter, task_handle);
        if (slot >= 0) {
            uint16_t index = static_cast<uint16_t>(slot);
            context = adapter->context_ids[index];
            generation = adapter->generations[index];
            adapter->active[index] = 0u;
        }
        adapter->unlock(adapter->user);
        if (context != 0u) {
            KymoTrace_TaskEnd(adapter->recorder, context, generation);
        }
    }
}

/*******************************************************************************
 * KymoFreeRtosTrace_SwitchedOut
 ******************************************************************************/
void KymoFreeRtosTrace_SwitchedOut(KymoFreeRtosTraceAdapter *adapter,
    uint8_t core_id, const void *task_handle)
{
    if ((adapter != nullptr) && (core_id < KYMO_TRACE_CORE_COUNT)) {
        adapter->lock(adapter->user);
        adapter->outgoing[core_id] = task_handle;
        adapter->unlock(adapter->user);
    }
}

/*******************************************************************************
 * KymoFreeRtosTrace_SwitchedIn
 ******************************************************************************/
void KymoFreeRtosTrace_SwitchedIn(KymoFreeRtosTraceAdapter *adapter,
    uint8_t core_id, const void *task_handle)
{
    if ((adapter != nullptr) && (core_id < KYMO_TRACE_CORE_COUNT)) {
        adapter->lock(adapter->user);
        uint32_t outgoing = contextFor(adapter, adapter->outgoing[core_id]);
        uint32_t incoming = contextFor(adapter, task_handle);
        adapter->outgoing[core_id] = task_handle;
        adapter->unlock(adapter->user);
        KymoTrace_Scheduler(adapter->recorder, outgoing, incoming);
    }
}
#endif
