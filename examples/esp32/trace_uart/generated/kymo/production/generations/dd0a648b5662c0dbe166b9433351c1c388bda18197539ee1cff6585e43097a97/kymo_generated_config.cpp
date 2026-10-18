/* Generated static runtime wiring; callbacks remain user-owned. */
#include <kymo_generated_config.h>
#if KYMO_EFFECTIVE_RUNTIME
extern "C" uint32_t kymo_port_clock_ms(void *);
extern "C" uint8_t kymo_port_sample(void *, uint8_t, KymoSample *);
extern "C" uint8_t kymo_port_write(void *, const uint8_t *, uint8_t);
extern "C" uint8_t kymo_port_busy(void *);
extern "C" void kymo_port_service(void *);
#endif

#if KYMO_EFFECTIVE_RUNTIME
static const KymoChannel kymo_channels[] = {

};
const KymoConfig kymo_generated_config = {
    kymo_channels, 0u,
    kymo_port_clock_ms, kymo_port_sample, kymo_port_write,
    kymo_port_busy, kymo_port_service,
    nullptr, nullptr,
    nullptr
};
 #endif
 
 