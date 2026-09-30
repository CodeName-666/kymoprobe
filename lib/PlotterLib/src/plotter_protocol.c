#include "plotter_protocol.h"
#include "common/embedded_bytes.h"
#include "common/embedded_crc.h"
#include <math.h>

/*******************************************************************************
 * point_valid
 ******************************************************************************/
static uint8_t point_valid(const PlotterDataPoint *point)
{
    uint8_t valid = 0;
    if (point != NULL) {
        valid = (uint8_t)(emb_u8_only_bits(point->flags, PLOTTER_ALLOWED_FLAGS) &&
            isfinite(point->value) &&
            (!emb_u8_has_any(point->flags, PLOTTER_FLAG_X) || isfinite(point->x)) &&
            (!emb_u8_has_any(point->flags, PLOTTER_FLAG_Z) || isfinite(point->z)));
    }
    return valid;
}

/*******************************************************************************
 * plotter_crc8
 ******************************************************************************/
uint8_t plotter_crc8(const uint8_t *data, size_t length)
{
    uint8_t result = emb_crc8_msb(data, length, 0x07u, 0u);
    return result;
}

/*******************************************************************************
 * plotter_frame_length
 ******************************************************************************/
size_t plotter_frame_length(uint8_t descriptor)
{
    size_t length = 0;
    if ((emb_u8_mask(descriptor, PLOTTER_DESCRIPTOR_VERSION_MASK) == PLOTTER_DESCRIPTOR_DATA) &&
        !emb_u8_has_any(descriptor, PLOTTER_DESCRIPTOR_RESERVED_MASK)) {
        length = PLOTTER_FRAME_MIN_SIZE;
        if (emb_u8_has_any(descriptor, PLOTTER_FLAG_X)) {
            length += 4u;
        }
        if (emb_u8_has_any(descriptor, PLOTTER_FLAG_Z)) {
            length += 4u;
        }
        if (emb_u8_has_any(descriptor, PLOTTER_FLAG_TIMESTAMP)) {
            length += 4u;
        }
    }
    return length;
}

/*******************************************************************************
 * plotter_encode_data
 ******************************************************************************/
size_t plotter_encode_data(const PlotterDataPoint *point,
                           uint8_t *output, size_t output_capacity)
{
    size_t result = 0;
    if ((output != NULL) && point_valid(point)) {
        uint8_t descriptor = EMB_U8_OR(PLOTTER_DESCRIPTOR_DATA, point->flags);
        size_t required = plotter_frame_length(descriptor);
        if ((required != 0u) && (output_capacity >= required)) {
            size_t offset = 4;
            output[0] = PLOTTER_SYNC_0;
            output[1] = PLOTTER_SYNC_1;
            output[2] = descriptor;
            output[3] = point->id;
            if (emb_u8_has_any(point->flags, PLOTTER_FLAG_X)) {
                emb_write_f32_le(output + offset, point->x);
                offset += 4u;
            }
            emb_write_f32_le(output + offset, point->value);
            offset += 4u;
            if (emb_u8_has_any(point->flags, PLOTTER_FLAG_Z)) {
                emb_write_f32_le(output + offset, point->z);
                offset += 4u;
            }
            if (emb_u8_has_any(point->flags, PLOTTER_FLAG_TIMESTAMP)) {
                emb_write_u32_le(output + offset, point->timestamp_ms);
                offset += 4u;
            }
            output[offset] = plotter_crc8(output + 2, offset - 2u);
            result = offset + 1u;
        }
    }
    return result;
}

/*******************************************************************************
 * frame_valid
 ******************************************************************************/
static uint8_t frame_valid(const uint8_t *frame, size_t length)
{
    uint8_t valid = 0;
    if ((frame != NULL) && (length >= PLOTTER_FRAME_MIN_SIZE)) {
        size_t expected = plotter_frame_length(frame[2]);
        if ((frame[0] == PLOTTER_SYNC_0) && (frame[1] == PLOTTER_SYNC_1) &&
            (expected != 0u) && (expected == length)) {
            valid = (uint8_t)(plotter_crc8(frame + 2, length - 3u) == frame[length - 1u]);
        }
    }
    return valid;
}

/* Only called after frame_valid: the descriptor proves all payload bounds. */

/*******************************************************************************
 * decode_payload
 ******************************************************************************/
static void decode_payload(const uint8_t *frame, PlotterDataPoint *point)
{
    size_t offset = 4;
    point->id = frame[3];
    point->flags = emb_u8_mask(frame[2], PLOTTER_ALLOWED_FLAGS);
    if (emb_u8_has_any(point->flags, PLOTTER_FLAG_X)) {
        point->x = emb_read_f32_le(frame + offset);
        offset += 4u;
    }
    point->value = emb_read_f32_le(frame + offset);
    offset += 4u;
    if (emb_u8_has_any(point->flags, PLOTTER_FLAG_Z)) {
        point->z = emb_read_f32_le(frame + offset);
        offset += 4u;
    }
    if (emb_u8_has_any(point->flags, PLOTTER_FLAG_TIMESTAMP)) {
        point->timestamp_ms = emb_read_u32_le(frame + offset);
    }
}

/*******************************************************************************
 * plotter_decode_data
 ******************************************************************************/
int plotter_decode_data(const uint8_t *frame, size_t frame_length,
                        PlotterDataPoint *output)
{
    int result = 0;
    if ((output != NULL) && frame_valid(frame, frame_length)) {
        PlotterDataPoint point = {0};
        decode_payload(frame, &point);
        if (point_valid(&point)) {
            *output = point;
            result = 1;
        }
    }
    return result;
}
