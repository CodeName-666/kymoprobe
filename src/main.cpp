#include <Arduino.h>
#include "math.h"
#include "plotter.h"


using namespace Plotter;

Plotter::Plotter p;

SwTimer loopTimer;
void setup() {
    Serial.begin(9600);
    p.init(Serial);
    loopTimer.setTime(1000);
    loopTimer.enable();
    loopTimer.start();
}



void loop() {


    if(loopTimer.isExeeded())
    {
        Serial.println("... LOOP ...");
        loopTimer.restart();
    }

    p.loop();



}