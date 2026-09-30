#include "../../../common/arduino_serial_config.h"

static PlotterContext plotter;

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup() {
    Serial.begin(115200); // CDC line coding; USB itself is not limited to this baud.
    (void)Plotter_Init(&plotter, &example_config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop() { (void)Plotter_Main(&plotter); }
