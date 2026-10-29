/* SPDX-License-Identifier: MIT */
#ifndef KYMO_FREERTOS_TRACE_HOOKS_H
#define KYMO_FREERTOS_TRACE_HOOKS_H

/** @file kymo_freertos_trace_hooks.h
 * @brief FreeRTOS trace macro bindings for the source-built adapter.
 * @details Include from the source-built kernel's FreeRTOSConfig.h after
 * declaring kymoFreeRtosTraceAdapter. Define KYMO_FREERTOS_TRACE_CORE_ID() to the
 * target's verified current-core helper. Existing traceTASK_* hooks must be
 * chained manually in the same macros before enabling this header.
 */
#include "kymo_freertos_trace_adapter.h"

#ifndef KYMO_FREERTOS_TRACE_CORE_ID
#error "Define KYMO_FREERTOS_TRACE_CORE_ID() for this source-built FreeRTOS port"
#endif

/** @brief Application-owned adapter instance used by the hook macros. */
extern KymoFreeRtosTraceAdapter kymoFreeRtosTraceAdapter;

#if (defined(traceTASK_CREATE) || defined(traceTASK_DELETE) || \
     defined(traceTASK_SWITCHED_OUT) || defined(traceTASK_SWITCHED_IN)) && \
    !defined(KYMO_FREERTOS_TRACE_HOOKS_CHAINS_EXISTING)
#error "Chain existing FreeRTOS traceTASK_* hooks explicitly before including this adapter"
#endif

/** @brief Bind FreeRTOS task creation to the KymoTrace adapter. */
#define traceTASK_CREATE(task_handle) \
    ((void)KymoFreeRtosTrace_TaskCreate(&kymoFreeRtosTraceAdapter, \
                                       (const void *)(task_handle)))
/** @brief Bind FreeRTOS task deletion to the KymoTrace adapter. */
#define traceTASK_DELETE(task_handle) \
    KymoFreeRtosTrace_TaskDelete(&kymoFreeRtosTraceAdapter, \
                                 (const void *)(task_handle))
/** @brief Capture the outgoing task on a scheduler switch. */
#define traceTASK_SWITCHED_OUT() \
    KymoFreeRtosTrace_SwitchedOut(&kymoFreeRtosTraceAdapter, \
        (uint8_t)KYMO_FREERTOS_TRACE_CORE_ID(), \
        (const void *)xTaskGetCurrentTaskHandle())
/** @brief Emit the outgoing/incoming task pair after a scheduler switch. */
#define traceTASK_SWITCHED_IN() \
    KymoFreeRtosTrace_SwitchedIn(&kymoFreeRtosTraceAdapter, \
        (uint8_t)KYMO_FREERTOS_TRACE_CORE_ID(), \
        (const void *)xTaskGetCurrentTaskHandle())

#endif
