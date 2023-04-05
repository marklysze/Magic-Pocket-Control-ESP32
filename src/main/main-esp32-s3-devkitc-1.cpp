#include <Arduino.h>

#define USING_TFT_ESPI 0    // Not using the TFT_eSPI graphics library <-- must include this in every main file, 0 = not using, 1 = using

void setup() {
    Serial.begin(115200);
}

void loop() {
    Serial.print("<O>");
    delay(1000);
}