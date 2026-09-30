#include "plotter_runtime.h"
#include <string.h>

/*******************************************************************************
 * config_valid
 ******************************************************************************/
static uint8_t config_valid(const PlotterConfig *config)
{
    uint8_t valid = 0;
    if (config != NULL) {
        valid = (uint8_t)((config->channels != NULL) &&
            (config->last_sample_ms != NULL) && (config->clock_ms != NULL) &&
            (config->sample != NULL) && (config->write != NULL) &&
            (config->channel_count > 0u) && (config->channel_count <= 256u));
        if (valid != 0u) {
            uint16_t i;
            uint8_t seen[32] = {0};
            for (i = 0; (i < config->channel_count) && (valid != 0u); ++i) {
                const PlotterChannel *ch = &config->channels[i];
                valid = (uint8_t)((ch->period_ms <= INT32_MAX) &&
                    emb_u8_only_bits(ch->flags, PLOTTER_ALLOWED_FLAGS) &&
                    !emb_bitset_test(seen, sizeof(seen), ch->id));
                if (valid != 0u) {
                    (void)emb_bitset_set(seen, sizeof(seen), ch->id);
                }
            }
        }
    }
    return valid;
}

/*******************************************************************************
 * Plotter_Init
 ******************************************************************************/
PlotterStatus Plotter_Init(PlotterContext *context, const PlotterConfig *config)
{
    PlotterStatus status = PLOTTER_BAD_CONFIG;
    if (context != NULL) {
        memset(context, 0, sizeof(*context));
        if (config_valid(config)) {
            uint16_t i;
            uint32_t now = config->clock_ms(config->clock_user);
            for (i = 0; i < config->channel_count; ++i) {
                config->last_sample_ms[i] = now - config->channels[i].period_ms;
            }
            context->start_ms = now;
            context->config = config;
            status = PLOTTER_OK;
        }
    }
    return status;
}

/*******************************************************************************
 * transmit
 ******************************************************************************/
static PlotterStatus transmit(PlotterContext *context)
{
    PlotterStatus status = PLOTTER_IO_ERROR;
    const PlotterConfig *config = context->config;
    uint8_t remaining = (uint8_t)(context->tx_length - context->tx_offset);
    uint8_t count = config->write(config->transport_user,
                                 context->tx + context->tx_offset, remaining);
    if (count > remaining) {
        context->fault = 1;
    } else {
        context->tx_offset = (uint8_t)(context->tx_offset + count);
        status = context->tx_offset == context->tx_length ? PLOTTER_OK : PLOTTER_BUSY;
    }
    return status;
}

/*******************************************************************************
 * sample_channel
 ******************************************************************************/
static PlotterStatus sample_channel(PlotterContext *context, uint16_t index,
                                    uint32_t now)
{
    PlotterStatus status = PLOTTER_SKIPPED;
    const PlotterConfig *config = context->config;
    const PlotterChannel *ch = &config->channels[index];
    PlotterSample sample = {0};
    config->last_sample_ms[index] = now;
    if (config->sample(config->sample_user, ch->id, &sample)) {
        PlotterDataPoint point = {0};
        point.id = ch->id;
        point.flags = ch->flags;
        point.x = sample.x;
        point.value = sample.value;
        point.z = sample.z;
        point.timestamp_ms = now - context->start_ms;
        context->tx_offset = 0;
        context->tx_length = (uint8_t)plotter_encode_data(&point, context->tx,
                                                        sizeof(context->tx));
        if (context->tx_length == 0u) {
            status = PLOTTER_BAD_SAMPLE;
        } else {
            status = transmit(context);
        }
    }
    return status;
}

/*******************************************************************************
 * sample_next_due
 ******************************************************************************/
static PlotterStatus sample_next_due(PlotterContext *context)
{
    PlotterStatus status = PLOTTER_IDLE;
    const PlotterConfig *config = context->config;
    uint32_t now = config->clock_ms(config->clock_user);
    uint16_t checked;
    uint8_t selected = 0;
    for (checked = 0; (checked < config->channel_count) && !selected; ++checked) {
        uint16_t index = context->next_channel;
        const PlotterChannel *ch = &config->channels[index];
        context->next_channel = (uint16_t)(index + 1u);
        if (context->next_channel == config->channel_count) {
            context->next_channel = 0;
        }
        if ((uint32_t)(now - config->last_sample_ms[index]) >= ch->period_ms) {
            selected = 1;
            status = sample_channel(context, index, now);
        }
    }
    return status;
}

/*******************************************************************************
 * Plotter_Main
 ******************************************************************************/
PlotterStatus Plotter_Main(PlotterContext *context)
{
    PlotterStatus status = PLOTTER_BAD_CONFIG;
    if ((context != NULL) && (context->config != NULL)) {
        const PlotterConfig *config = context->config;
        if (config->service != NULL) {
            config->service(config->transport_user);
        }
        if (context->fault) {
            status = PLOTTER_IO_ERROR;
        } else if ((config->busy != NULL) && config->busy(config->transport_user)) {
            status = PLOTTER_BUSY;
        } else if (context->tx_offset < context->tx_length) {
            status = transmit(context);
        } else {
            status = sample_next_due(context);
        }
    }
    return status;
}
