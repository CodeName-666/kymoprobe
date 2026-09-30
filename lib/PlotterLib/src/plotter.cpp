#include "plotter.h"

/*******************************************************************************
 * Plotter::Plotter
 ******************************************************************************/
Plotter::Plotter()
    : stream(nullptr), startTimeMs(0), useTimestamp(true), getMillisecond(nullptr)
{
}


/*******************************************************************************
 * Plotter::Plotter
 ******************************************************************************/
Plotter::Plotter(PlotterStream &outputStream, bool enableTimestamp)
    : stream(&outputStream), startTimeMs(0), useTimestamp(enableTimestamp),
      getMillisecond(nullptr)
{
}

/*******************************************************************************
 * Plotter::~Plotter
 ******************************************************************************/
Plotter::~Plotter()
{
}

/*******************************************************************************
 * Plotter::begin
 ******************************************************************************/
void Plotter::begin(PlotterStream &outputStream, bool enableTimestamp)
{
    stream = &outputStream;
    useTimestamp = enableTimestamp;
    startTimeMs = 0;
    txLength = txOffset = 0;
    txFault = false;
}


/*******************************************************************************
 * Plotter::setStartTime
 ******************************************************************************/
void Plotter::setStartTime(uint32_t startTime)
{
    startTimeMs = startTime;
}

/*******************************************************************************
 * Plotter::setMillisecondCallback
 ******************************************************************************/
void Plotter::setMillisecondCallback(GetMillisecondCallback callback)
{
    getMillisecond = callback;
}

/*******************************************************************************
 * Plotter::sendFrame
 ******************************************************************************/
bool Plotter::sendFrame(uint8_t channelId,
                        uint8_t flags,
                        float xValue,
                        float yValue,
                        float zValue,
                        bool includeTimestamp)
{
    bool accepted = false;
    if (stream && !txFault && (txOffset == txLength) && !stream->busy()) {
        PlotterDataPoint point = {};
        point.id = channelId;
        point.flags = flags;
        point.x = xValue;
        point.value = yValue;
        point.z = zValue;
        if (includeTimestamp && useTimestamp && getMillisecond) {
            point.flags = EMB_U8_OR(point.flags, PLOTTER_FLAG_TIMESTAMP);
            point.timestamp_ms = (uint32_t)getMillisecond() - startTimeMs;
        }
        const size_t length = plotter_encode_data(&point, buffer, sizeof(buffer));
        if (length > 0u) {
            txLength = static_cast<uint8_t>(length);
            txOffset = 0;
            (void)flush();
            accepted = !txFault;
        }
    }
    return accepted;
}

/*******************************************************************************
 * Plotter::send
 ******************************************************************************/
bool Plotter::send(uint8_t channelId, float yValue, bool includeTimestamp)
{
    return sendFrame(channelId, 0, 0.0f, yValue, 0.0f, includeTimestamp);
}

/*******************************************************************************
 * Plotter::send
 ******************************************************************************/
bool Plotter::send(uint8_t channelId, float yValue)
{
    return send(channelId, yValue, false);
}

/*******************************************************************************
 * Plotter::send2D
 ******************************************************************************/
bool Plotter::send2D(uint8_t channelId, float xValue, float yValue, bool includeTimestamp)
{
    return sendFrame(channelId, PLOTTER_FLAG_X, xValue, yValue, 0.0f, includeTimestamp);
}

/*******************************************************************************
 * Plotter::send2D
 ******************************************************************************/
bool Plotter::send2D(uint8_t channelId, float xValue, float yValue)
{
    return send2D(channelId, xValue, yValue, false);
}

/*******************************************************************************
 * Plotter::send3D
 ******************************************************************************/
bool Plotter::send3D(uint8_t channelId,
                     float xValue,
                     float yValue,
                     float zValue,
                     bool includeTimestamp)
{
    return sendFrame(channelId,
              EMB_U8_OR(PLOTTER_FLAG_X, PLOTTER_FLAG_Z),
              xValue,
              yValue,
              zValue,
              includeTimestamp);
}

/*******************************************************************************
 * Plotter::send3D
 ******************************************************************************/
bool Plotter::send3D(uint8_t channelId, float xValue, float yValue, float zValue)
{
    return send3D(channelId, xValue, yValue, zValue, false);
}

/*******************************************************************************
 * Plotter::setTimestampEnabled
 ******************************************************************************/
void Plotter::setTimestampEnabled(bool enable)
{
    useTimestamp = enable;
}

/*******************************************************************************
 * Plotter::isTimestampEnabled
 ******************************************************************************/
bool Plotter::isTimestampEnabled() const
{
    return useTimestamp;
}

/*******************************************************************************
 * Plotter::flush
 ******************************************************************************/
bool Plotter::flush()
{
    bool complete = false;
    if (stream && !txFault && !stream->busy()) {
        if (txOffset == txLength) {
            complete = true;
        } else {
            const size_t remaining = txLength - txOffset;
            const size_t written = stream->write(buffer + txOffset, remaining);
            if (written > remaining) {
                txFault = true;
            } else {
                txOffset = static_cast<uint8_t>(txOffset + written);
                complete = txOffset == txLength;
            }
        }
    }
    return complete;
}
