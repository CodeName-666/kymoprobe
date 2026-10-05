#include <assert.h>
#include <string.h>
#include "plotter.h"
#include "../../examples/stm32/adapters/plotter_stm32.h"

static bool cdcBusy;
static const uint8_t *cdcBuffer;

/*******************************************************************************
 * cdcTransmit
 ******************************************************************************/
static uint8_t cdcTransmit(uint8_t *data, uint16_t) {
    cdcBuffer = data;
    cdcBusy = true;
    return 0;
}

/*******************************************************************************
 * cdcIsBusy
 ******************************************************************************/
static bool cdcIsBusy() { return cdcBusy; }

class ShortStream : public PlotterStream {
public:
    uint8_t bytes[128] = {};
    size_t length = 0;
    size_t limit = 3;
    bool blocked = false;

    /***************************************************************************
     * busy
     **************************************************************************/
    bool busy() const override { return blocked; }

    /***************************************************************************
     * write
     **************************************************************************/
    size_t write(const uint8_t *data, size_t count) override {
        if (count > limit) count = limit;
        memcpy(bytes + length, data, count);
        length += count;
        return count;
    }
};

/*******************************************************************************
 * main
 ******************************************************************************/
int main() {
    ShortStream stream;
    Plotter p(stream);
    assert(p.send(7, 1.0f)); /* accepted; partial write remains pending */
    stream.blocked = true;
    assert(!p.send(9, 2.0f));
    assert(stream.length == 3);
    stream.blocked = false;
    assert(!p.flush());
    assert(p.flush());
    assert(stream.length == 9);
    static const uint8_t golden[] = {0xa5,0x5a,PLOTTER_ENABLE_CRC ? 0x40 : 0x41,7,0,0,0x80,0x3f,
        PLOTTER_ENABLE_CRC ? 0x54 : 0};
    assert(memcmp(stream.bytes, golden, 9) == 0);
    stream.limit = 21;
    assert(p.send(9, 2.0f));
    assert(stream.length == 18);

    CDCStream cdc(cdcTransmit, cdcIsBusy);
    Plotter usb(cdc);
    assert(usb.send(7, 1.0f));
    assert(!usb.send(9, 2.0f));
    assert(!usb.flush());
    assert(memcmp(cdcBuffer, golden, 9) == 0);
    cdcBusy = false;
    assert(usb.flush());
    assert(usb.send(9, 2.0f));
    assert(cdcBuffer[3] == 9);
    return 0;
}
