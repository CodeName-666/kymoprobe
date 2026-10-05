#include <assert.h>
#include "plotter.h"

class FeatureStream : public PlotterStream {
public:
    unsigned writes = 0;
    uint8_t descriptor = 0;

    /***************************************************************************
     * FeatureStream::write
     **************************************************************************/
    size_t write(const uint8_t *data, size_t length) override
    {
        ++writes;
        descriptor = data[2];
        return length;
    }
};

/*******************************************************************************
 * main
 ******************************************************************************/
int main()
{
    FeatureStream stream;
    Plotter sender(stream);
    assert(!sender.isTimestampEnabled());
    sender.setTimestampEnabled(true);
    assert(!sender.isTimestampEnabled());
    assert(sender.send(7u, 1.0f));
    assert(stream.descriptor == 0x41u);
    assert(!sender.send2D(7u, 2.0f, 1.0f));
    assert(!sender.send3D(7u, 2.0f, 1.0f, 3.0f));
    assert(!sender.send(7u, 1.0f, true));
    assert(stream.writes == 1u);
    assert(sender.flush());
    return 0;
}
