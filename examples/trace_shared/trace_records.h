/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file trace_records.h @brief Demo's canonical trace dictionary and metadata. */
#ifndef KYMO_TRACE_DEMO_RECORDS_H
#define KYMO_TRACE_DEMO_RECORDS_H

#define KYMO_DEMO_CONTEXT_MAIN 1u
#define KYMO_DEMO_SYMBOL_MAIN_LOOP 1u
#define KYMO_DEMO_SYMBOL_WORK 2u
#define KYMO_DEMO_SYMBOL_ITERATION 3u
#define KYMO_DEMO_DICTIONARY_GENERATION 1u

static const char kymoDemoDictionary[] =
    R"json({"contexts":[{"id":1,"kind":"main","name":"main"}],"generation":1,"schema_version":1,"symbols":[{"id":1,"kind":"scope","name":"main_loop"},{"id":2,"kind":"scope","name":"simulated_work"},{"id":3,"kind":"marker","name":"iteration"}]})json";

static const char kymoDemoSessionMetadata[] =
    R"json({"build_id":"trace-demo-v1","clock":{"counter_bits":64,"domain_id":1,"frequency_hz":1000000,"reset_rule":"new_session","smp_synchronized":true},"core_count":2,"device_id":"kymoprobe-esp32-trace-demo","dictionary_hash":"db347ee5fa037aff5fd2b5abc5e152fef23ec25b66b46a03add3902c12d115fc","event_schema_version":1,"features":{"flow":false,"isr":false,"scheduler":false,"trace":true},"port_capabilities":{"core_count":2,"core_state":false,"isr":false,"scheduler":false},"schema_version":1,"wire_version":2})json";

#endif
