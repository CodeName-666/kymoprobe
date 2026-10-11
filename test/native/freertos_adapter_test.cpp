/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial */
#include "kymo_freertos_trace_adapter.h"
#include <cstdio>

#define KYMO_FREERTOS_TRACE_CORE_ID() 0u
#include "kymo_freertos_trace_hooks.h"

KymoFreeRtosTraceAdapter kymoFreeRtosTraceAdapter;

namespace {
KymoTraceRecorder recorder;
uint64_t clockValue = 1u;
const void *currentTask = nullptr;
int taskA = 1;
int taskB = 2;
int taskFromHook = 4;

/*******************************************************************************
 * mockClock
 ******************************************************************************/
uint64_t mockClock(void *)
{
    return clockValue++;
}

/*******************************************************************************
 * mockCore
 ******************************************************************************/
uint8_t mockCore(void *)
{
    return 0u;
}

/*******************************************************************************
 * mockContext
 ******************************************************************************/
uint32_t mockContext(void *)
{
    return 1u;
}

/*******************************************************************************
 * mockLock
 ******************************************************************************/
void mockLock(void *)
{
}

/*******************************************************************************
 * mockUnlock
 ******************************************************************************/
void mockUnlock(void *)
{
}

/*******************************************************************************
 * mockContextForTask
 ******************************************************************************/
uint32_t mockContextForTask(void *, const void *taskHandle)
{
    uint32_t result = 0u;
    if (taskHandle == &taskA) {
        result = 42u;
    } else if (taskHandle == &taskB) {
        result = 43u;
    } else if (taskHandle == &taskFromHook) {
        result = 44u;
    }
    return result;
}

/*******************************************************************************
 * xTaskGetCurrentTaskHandle
 ******************************************************************************/
const void *xTaskGetCurrentTaskHandle(void)
{
    return currentTask;
}

/*******************************************************************************
 * check
 ******************************************************************************/
int check(bool condition, const char *message)
{
    int result = 0;
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        result = 1;
    }
    return result;
}
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    int result = 0;
    KymoTraceConfig config = {};
    KymoTraceEvent event = {};
    config.clock_ticks = mockClock;
    config.core_id = mockCore;
    config.current_context = mockContext;
    config.critical_enter = mockLock;
    config.critical_exit = mockUnlock;
    if (KymoTrace_Init(&recorder, &config) == 0u) {
        result = 1;
    }
    if ((result == 0) && (KymoFreeRtosTrace_Init(&kymoFreeRtosTraceAdapter, &recorder,
            mockLock, mockUnlock, nullptr) == 0u)) {
        result = 1;
    }
    KymoFreeRtosTrace_SetContextResolver(&kymoFreeRtosTraceAdapter,
                                        mockContextForTask);
    uint32_t first = KymoFreeRtosTrace_TaskCreate(&kymoFreeRtosTraceAdapter, &taskA);
    uint32_t second = KymoFreeRtosTrace_TaskCreate(&kymoFreeRtosTraceAdapter, &taskB);
    if (result == 0) {
        result |= check((first != 0u) && (second != 0u) && (first != second),
                        "task context IDs were not unique");
        KymoFreeRtosTrace_SwitchedOut(&kymoFreeRtosTraceAdapter, 0u, &taskA);
        KymoFreeRtosTrace_SwitchedIn(&kymoFreeRtosTraceAdapter, 0u, &taskB);
        KymoFreeRtosTrace_TaskDelete(&kymoFreeRtosTraceAdapter, &taskA);
        int taskReused = 3;
        uint32_t recycledHandleContext =
            KymoFreeRtosTrace_TaskCreate(&kymoFreeRtosTraceAdapter, &taskReused);
        result |= check(recycledHandleContext > second,
                        "task context ID was recycled after deletion");
        result |= check(KymoTrace_Pop(&recorder, &event) != 0u,
                        "task create event was not emitted");
        result |= check(event.event_type == KYMO_TRACE_EVENT_TASK_CREATE,
                        "first event was not TASK_CREATE");
        result |= check(KymoTrace_Pop(&recorder, &event) != 0u,
                        "second task create event was not emitted");
        result |= check(KymoTrace_Pop(&recorder, &event) != 0u,
                        "scheduler transition was not emitted");
        result |= check(event.event_type == KYMO_TRACE_EVENT_TASK_SWITCH,
                        "scheduler event was not TASK_SWITCH");
        result |= check(event.context_id == first && event.arg == second,
                        "scheduler transition has wrong task identities");
        result |= check(KymoFreeRtosTrace_TaskCreate(
            &kymoFreeRtosTraceAdapter, &taskA) == first,
            "declared context ID changed on task recreation");
        (void)KymoTrace_Pop(&recorder, &event);
        result |= check(event.event_type == KYMO_TRACE_EVENT_TASK_END,
                        "task deletion event was not emitted");
        (void)KymoTrace_Pop(&recorder, &event);
        result |= check(event.event_type == KYMO_TRACE_EVENT_TASK_CREATE,
                        "fallback task creation event was not emitted");
        (void)KymoFreeRtosTrace_TaskCreate(&kymoFreeRtosTraceAdapter,
                                            &taskA);
        (void)KymoTrace_Pop(&recorder, &event);
        result |= check((event.event_type == KYMO_TRACE_EVENT_TASK_CREATE) &&
                        (event.arg == 2u),
                        "recreated static context generation did not advance");
        (void)traceTASK_CREATE(&taskFromHook);
        currentTask = &taskFromHook;
        traceTASK_SWITCHED_OUT();
        currentTask = &taskB;
        traceTASK_SWITCHED_IN();
        traceTASK_DELETE(&taskFromHook);
    }
    if (result == 0) {
        std::puts("FreeRTOS adapter mock passed");
    }
    return result;
}
