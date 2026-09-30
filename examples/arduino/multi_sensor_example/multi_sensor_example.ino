#define PLOTTER_EXAMPLE_MULTIDIMENSIONAL 1
#include "../../common/arduino_serial_config.h"

static PlotterContext plotter;

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup() {
    Serial.begin(115200);
    (void)Plotter_Init(&plotter, &example_config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop() { (void)Plotter_Main(&plotter); }
