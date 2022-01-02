#include <Arduino.h>
#include "math.h"
#include "Stream.h"



Stream* stream;
uint32_t counter;


void setup() {
  Serial.begin(9600);
  stream = &Serial;
  counter = 0;
}

void loop() {


  double x = sin(counter);
  counter++;
  stream->printf(" t = %i | X = %f \n", millis(), x);
  delayMicroseconds(500);


}