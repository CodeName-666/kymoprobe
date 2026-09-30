/**
 * @brief Portable codec for Plotter binary protocol v6.
 *
 * @details Frames contain sync, descriptor, channel, optional X, mandatory Y, optional Z,
 * optional timestamp and CRC. Multi-byte payloads are little-endian. The codec
 * allocates no memory and never transmits the native structure layout.
 * @file plotter_protocol.h
 * @defgroup plotter_protocol Wire protocol and codec
 * @{
 * @par Usage
 * Include this header when encoding or decoding complete frames.
 * @code{.c}
 * PlotterDataPoint point = {0};
 * uint8_t frame[PLOTTER_FRAME_MAX_SIZE];
 * point.id = 7; point.value = 1.0f;
 * size_t length = plotter_encode_data(&point, frame, sizeof(frame));
 * @endcode
 */
#ifndef PLOTTER_PROTOCOL_H
/**
 * @brief Include guard for plotter_protocol.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define PLOTTER_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include "common/embedded_bits.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name Protocol constants
 * @{ */
/**
 * @brief Human-facing major protocol revision.
 *
 * @details Revision 6 names the compact binary protocol; this is not the descriptor version field.
 * @par Usage
 * Use for diagnostics or firmware compatibility information.
 */
#define PLOTTER_PROTOCOL_VERSION_MAJOR 6

/**
 * @brief Binary descriptor version before placement in bits 7..6.
 *
 * @details Version 1 is encoded by PLOTTER_DESCRIPTOR_DATA. Other versions are rejected.
 * @par Usage
 * Compare supported protocol versions without changing wire constants.
 */
#define PLOTTER_WIRE_VERSION 1

/**
 * @brief First synchronization byte at frame offset zero.
 *
 * @details Followed by PLOTTER_SYNC_1. Synchronization bytes are excluded from the CRC.
 * @par Usage
 * Search byte streams for the two-byte sync marker before framing.
 */
#define PLOTTER_SYNC_0 0xA5u

/**
 * @brief Second synchronization byte at frame offset one.
 *
 * @details Must immediately follow PLOTTER_SYNC_0; neither byte is a text delimiter.
 * @par Usage
 * Use with PLOTTER_SYNC_0 when locating a candidate frame.
 */
#define PLOTTER_SYNC_1 0x5Au

/**
 * @brief Base descriptor for wire version 1 and a data message.
 *
 * @details Bits 7..6 contain 01; message-type bits 5..4 contain 00. Optional flags are combined with this byte.
 * @par Usage
 * The encoder combines this base with PlotterDataPoint::flags.
 */
#define PLOTTER_DESCRIPTOR_DATA 0x40u

/**
 * @brief Descriptor bit indicating a four-byte X coordinate.
 *
 * @details Adds one finite binary32 value before the mandatory Y/value field.
 * @par Usage
 * Set in PlotterChannel::flags or PlotterDataPoint::flags for XY or XYZ measurements.
 */
#define PLOTTER_FLAG_X 0x08u

/**
 * @brief Descriptor bit indicating a four-byte Z coordinate.
 *
 * @details Adds a finite binary32 value after Y. Z can also be used without X.
 * @par Usage
 * Combine with PLOTTER_FLAG_X for three-dimensional measurements.
 */
#define PLOTTER_FLAG_Z 0x04u

/**
 * @brief Descriptor bit indicating relative uint32 milliseconds.
 *
 * @details Adds four bytes after all coordinates. The counter wraps modulo 2^32; the app converts it to seconds.
 * @par Usage
 * Set on a channel to include time relative to Plotter_Init.
 */
#define PLOTTER_FLAG_TIMESTAMP 0x02u

/**
 * @brief Mask of every supported optional payload field.
 *
 * @details Only X, Z and timestamp flags are permitted. Every combination of those bits is valid.
 * @par Usage
 * Use for XYZ with timestamps or to validate a supplied flag byte.
 */
#define PLOTTER_ALLOWED_FLAGS EMB_U8_OR(PLOTTER_FLAG_X, EMB_U8_OR(PLOTTER_FLAG_Z, PLOTTER_FLAG_TIMESTAMP))

/**
 * @brief Mask selecting the two wire-version bits.
 *
 * @details The masked descriptor must equal PLOTTER_DESCRIPTOR_DATA for a supported frame.
 * @par Usage
 * Used by plotter_frame_length before accepting a descriptor.
 */
#define PLOTTER_DESCRIPTOR_VERSION_MASK 0xC0u

/**
 * @brief Mask selecting unsupported message-type and reserved bits.
 *
 * @details Combines bits 5, 4 and 0. Any set bit makes the descriptor invalid in this revision.
 * @par Usage
 * Validate descriptor bytes with emb_u8_has_any.
 */
#define PLOTTER_DESCRIPTOR_RESERVED_MASK 0x31u

/**
 * @brief Size in bytes of a Y-only frame.
 *
 * @details Two sync bytes, descriptor, channel, binary32 Y and CRC. No optional fields.
 * @par Usage
 * Minimum complete-frame bound for decoding.
 */
#define PLOTTER_FRAME_MIN_SIZE 9u

/**
 * @brief Maximum complete-frame size in bytes.
 *
 * @details Includes X, Y, Z, timestamp and framing. A single buffer of this size supports all flag combinations.
 * @par Usage
 * Declare uint8_t frame[PLOTTER_FRAME_MAX_SIZE].
 */
#define PLOTTER_FRAME_MAX_SIZE 21u

/** @} */

/**
 * @brief Native application representation of one measurement.
 *
 * @details Input to the encoder and output of successful decoding. Padding and host byte order
 * are never sent. Zero-initialize before filling fields; absent decoded fields become zero.
 * All selected coordinates must be finite IEEE-754 binary32 values.
 * @par Usage
 * Set id/value and optional flags/fields, then call plotter_encode_data.
 */
typedef struct
{
    /**
     * @brief Channel identifier in the inclusive range 0..255.
     *
     * @details Input to encoding; output from decoding.
     * @par Usage
     * point.id = 7;
     */
    uint8_t id;
    /**
     * @brief Presence flags for X, Z and timestamp.
     *
     * @details Input/output; only PLOTTER_ALLOWED_FLAGS bits are accepted. Zero selects Y only.
     * @par Usage
     * point.flags = PLOTTER_ALLOWED_FLAGS;
     */
    uint8_t flags;
    /**
     * @brief Optional X coordinate.
     *
     * @details Input/output; relevant only when PLOTTER_FLAG_X is set. Decoded absent X is zero.
     * @par Usage
     * Set point.x for Cartesian measurements.
     */
    float x;
    /**
     * @brief Mandatory Y coordinate or scalar measurement.
     *
     * @details Input/output; must be finite even when no optional fields are selected.
     * @par Usage
     * point.value = temperature;
     */
    float value;
    /**
     * @brief Optional Z coordinate.
     *
     * @details Input/output; relevant only when PLOTTER_FLAG_Z is set. Decoded absent Z is zero.
     * @par Usage
     * Set point.z for XYZ or YZ measurements.
     */
    float z;
    /**
     * @brief Optional timestamp in milliseconds relative to the sender epoch.
     *
     * @details Input/output; used only with PLOTTER_FLAG_TIMESTAMP. Wraps modulo 2^32.
     * @par Usage
     * Use elapsed milliseconds rather than Unix time.
     */
    uint32_t timestamp_ms;
} PlotterDataPoint;

/** @name Codec operations
 * @{ */
/**
 * @brief Calculate CRC-8/ATM over a byte range.
 *
 * @details Polynomial 0x07, initial zero, no reflection and no final XOR. There are eight
 * bit iterations per input byte. Null data is treated as empty and produces zero.
 * @param[in] data Readable byte range; may be NULL for the defined empty-input behavior.
 * @param[in] length Number of bytes to read; must fit the actual data allocation when data is non-null.
 * @return CRC byte in 0..255.
 * @par Usage
 * plotter_crc8(frame + 2, frame_length - 3) covers descriptor through payload.
 */
uint8_t plotter_crc8(const uint8_t *data, size_t length);

/**
 * @brief Encode a validated point into a caller-owned byte buffer.
 *
 * @details No allocation, text conversion or struct casting. Validation failures leave the output
 * buffer unchanged. The result depends only on the selected optional fields.
 * @param[in] point Measurement to encode; NULL is rejected. Selected coordinates must be finite.
 * @param[out] output Writable frame buffer; NULL is rejected. Must not overlap point.
 * @param[in] output_capacity Actual writable capacity in bytes; 21 bytes handles every point.
 * @return Encoded length (9, 13, 17 or 21), or zero for invalid input/insufficient capacity.
 * @par Usage
 * size_t n = plotter_encode_data(&point, frame, sizeof(frame));
 * Transmit only frame[0..n-1] when n is nonzero.
 */
size_t plotter_encode_data(const PlotterDataPoint *point,
                           uint8_t *output,
                           size_t output_capacity);

/**
 * @brief Validate and decode exactly one complete binary frame.
 *
 * @details Checks synchronization, descriptor, exact length, CRC and finite selected coordinates.
 * Only a fully valid result is committed; failure leaves output unchanged. The caller
 * performs stream reassembly before calling this function.
 * @param[in] frame Readable frame bytes; NULL is rejected.
 * @param[in] frame_length Actual readable length; must match the descriptor-derived size exactly.
 * @param[out] output Destination measurement, or NULL to reject. Unselected fields are zero on success.
 * @return One on success; zero on validation failure.
 * @par Usage
 * Consume point only when plotter_decode_data(frame, length, &point) returns nonzero.
 */
int plotter_decode_data(const uint8_t *frame,
                        size_t frame_length,
                        PlotterDataPoint *output);

/**
 * @brief Derive complete-frame length from a supported descriptor.
 *
 * @details Unsupported versions, message types or reserved bits yield zero. No payload access is performed.
 * @param[in] descriptor Descriptor byte from offset two of a candidate frame.
 * @return 9, 13, 17 or 21 bytes for supported layouts; zero otherwise.
 * @par Usage
 * Wait for plotter_frame_length(descriptor) bytes before complete-frame decoding.
 */
size_t plotter_frame_length(uint8_t descriptor);
/** @} */

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* PLOTTER_PROTOCOL_H */
