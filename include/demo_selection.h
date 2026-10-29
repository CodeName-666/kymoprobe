/* SPDX-License-Identifier: MIT */
/** @file demo_selection.h
 * @brief Select one application and its matching library features at build time.
 * @details This header is force-included in application and library translation
 * units. Compiler definitions override defaults. Generated UART/trace profiles
 * supply their own Core settings instead. No application resources are allocated.
 * @par Usage Select a PlatformIO environment, or change KYMO_DEMO_MODE below
 * and build the default nodemcu-32s environment.
 */
#ifndef KYMO_PROBE_DEMO_SELECTION_H
/** @brief Header guard. */
#define KYMO_PROBE_DEMO_SELECTION_H

/** @brief Two waveform channels over UART. */
#define KYMO_DEMO_UART 0
/** @brief Waveform over MQTT only. */
#define KYMO_DEMO_MQTT 1
/** @brief Waveform over TCP only. */
#define KYMO_DEMO_TCP 2
/** @brief Independent TCP and MQTT runtimes. */
#define KYMO_DEMO_NETWORK 3
/** @brief Maximum scalar UART throughput on 256 channels. */
#define KYMO_DEMO_MAX_USB 4
/** @brief Maximum scalar TCP throughput on 256 channels. */
#define KYMO_DEMO_MAX_TCP 5
/** @brief Maximum XYZ TCP throughput on 256 channels. */
#define KYMO_DEMO_MAX_XYZ 6
/** @brief C++ sender, XYZ/time, CRC and local decoder verification. */
#define KYMO_DEMO_FEATURES 7
/** @brief UART runtime driven by generated development/production settings. */
#define KYMO_DEMO_GENERATED 8
/** @brief Generated scope/marker trace over UART wire v2. */
#define KYMO_DEMO_TRACE 9
/** @brief Generated trace with explicit send/receive flow correlation. */
#define KYMO_DEMO_TRACE_FLOW 10
/** @brief 256-channel load over UDP datagrams, without backpressure. */
#define KYMO_DEMO_MAX_UDP 11

#ifndef KYMO_DEMO_MODE
/** @brief Default application; override via a named PlatformIO environment. */
#define KYMO_DEMO_MODE KYMO_DEMO_UART
#endif
#if (KYMO_DEMO_MODE < KYMO_DEMO_UART) || (KYMO_DEMO_MODE > KYMO_DEMO_MAX_UDP)
#error "Unknown KYMO_DEMO_MODE"
#endif

/* Generated profiles remain the authority for all Core feature switches. */
#if (KYMO_DEMO_MODE != KYMO_DEMO_GENERATED) && \
    (KYMO_DEMO_MODE != KYMO_DEMO_TRACE) && \
    (KYMO_DEMO_MODE != KYMO_DEMO_TRACE_FLOW)
#ifndef KYMO_ENABLE_TIMESTAMP
/** @brief Measurement demos include millisecond timestamps. */
#define KYMO_ENABLE_TIMESTAMP 1
#endif
#if (KYMO_DEMO_MODE == KYMO_DEMO_FEATURES) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_MAX_XYZ)
#ifndef KYMO_ENABLE_X
/** @brief Enable X for feature and XYZ load demos. */
#define KYMO_ENABLE_X 1
#endif
#ifndef KYMO_ENABLE_Z
/** @brief Enable Z for feature and XYZ load demos. */
#define KYMO_ENABLE_Z 1
#endif
#endif
#if (KYMO_DEMO_MODE == KYMO_DEMO_MAX_USB) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_MAX_TCP) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_MAX_XYZ) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_MAX_UDP)
#ifndef KYMO_MAX_CHANNELS
/** @brief Size the runtime schedule for all 256 stress channels. */
#define KYMO_MAX_CHANNELS 256u
#endif
#endif
#if KYMO_DEMO_MODE == KYMO_DEMO_FEATURES
#ifndef KYMO_ENABLE_CRC
/** @brief Enable CRC in the feature demonstration. */
#define KYMO_ENABLE_CRC 1
#endif
#ifndef KYMO_ENABLE_DECODER
/** @brief Enable local frame decoding in the feature demonstration. */
#define KYMO_ENABLE_DECODER 1
#endif
#ifndef KYMO_ENABLE_CPP
/** @brief Enable the C++ push API in the feature demonstration. */
#define KYMO_ENABLE_CPP 1
#endif
#endif
#endif

#ifndef KYMO_DEMO_NETWORK_ENABLE
/** @brief Compile the selected network application. */
#define KYMO_DEMO_NETWORK_ENABLE ((KYMO_DEMO_MODE == KYMO_DEMO_MQTT) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_TCP) || (KYMO_DEMO_MODE == KYMO_DEMO_NETWORK))
#endif
/** @brief Include the TCP path for TCP or combined mode. */
#define KYMO_DEMO_TCP_ENABLE ((KYMO_DEMO_MODE == KYMO_DEMO_TCP) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_NETWORK))
/** @brief Include the MQTT path for MQTT or combined mode. */
#define KYMO_DEMO_MQTT_ENABLE ((KYMO_DEMO_MODE == KYMO_DEMO_MQTT) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_NETWORK))
#ifndef KYMO_STRESS_ENABLE
/** @brief Compile the maximum-throughput application. */
#define KYMO_STRESS_ENABLE ((KYMO_DEMO_MODE == KYMO_DEMO_MAX_USB) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_MAX_TCP) || (KYMO_DEMO_MODE == KYMO_DEMO_MAX_XYZ) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_MAX_UDP))
#endif
#ifndef KYMO_STRESS_TCP_ENABLE
/** @brief Select TCP for the TCP maximum-throughput profiles. */
#define KYMO_STRESS_TCP_ENABLE ((KYMO_DEMO_MODE == KYMO_DEMO_MAX_TCP) || \
    (KYMO_DEMO_MODE == KYMO_DEMO_MAX_XYZ))
#endif
#ifndef KYMO_STRESS_UDP_ENABLE
/** @brief Select UDP datagrams for the UDP maximum-throughput profile. */
#define KYMO_STRESS_UDP_ENABLE (KYMO_DEMO_MODE == KYMO_DEMO_MAX_UDP)
#endif
#ifndef KYMO_STRESS_XYZ_ENABLE
/** @brief Select the largest DATA layout in the XYZ load profile. */
#define KYMO_STRESS_XYZ_ENABLE (KYMO_DEMO_MODE == KYMO_DEMO_MAX_XYZ)
#endif

#if !defined(__ASSEMBLER__)
/** @brief Initialize the selected UART runtime. @note Not ISR-safe. */
void demoUartInit(void);
/** @brief Advance UART acquisition/transmission once. @note Not ISR-safe. */
void demoUartMain(void);
/** @brief Initialize selected network transports. @note Not ISR-safe. */
void demoNetworkInit(void);
/** @brief Advance network sampling/connections once. @note Not ISR-safe. */
void demoNetworkMain(void);
/** @brief Initialize the 256-channel load generator. @note Not ISR-safe. */
void demoStressInit(void);
/** @brief Run a bounded load-generator step. @note Not ISR-safe. */
void demoStressMain(void);
/** @brief Initialize the feature demonstration. @note Not ISR-safe. */
void demoFeaturesInit(void);
/** @brief Advance feature transmission once. @note Not ISR-safe. */
void demoFeaturesMain(void);
/** @brief Initialize the generated UART profile. @note Not ISR-safe. */
void demoGeneratedInit(void);
/** @brief Advance the generated runtime once. @note Not ISR-safe. */
void demoGeneratedMain(void);
/** @brief Initialize the generated trace session. @note Not ISR-safe. */
void demoTraceInit(void);
/** @brief Advance instrumentation and trace transport. @note Not ISR-safe. */
void demoTraceMain(void);
#endif /* !__ASSEMBLER__ */

#endif /* KYMO_PROBE_DEMO_SELECTION_H */
