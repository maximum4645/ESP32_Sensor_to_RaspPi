#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Starting test 1");
}

void loop() {
    Serial.println("Test 1");
    delay(2000);
}
