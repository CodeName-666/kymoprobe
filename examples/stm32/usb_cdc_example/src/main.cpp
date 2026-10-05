#include "../../../common/arduino_serial_config.h"

static KymoContext kymo;

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup() {
    Serial.begin(115200); // CDC line coding; USB itself is not limited to this baud.
    (void)Kymo_Init(&kymo, &example_config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop() { (void)Kymo_Main(&kymo); }
