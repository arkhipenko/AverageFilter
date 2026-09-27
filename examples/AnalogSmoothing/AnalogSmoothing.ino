//
// AnalogSmoothing: smooth analogRead() values with a moving average.
//
// Shows a heap-allocated window, a wider total type and rounding.
// 64 samples of up to 1023 add up to 65472, more than a 16-bit int holds
// on AVR, so the running total is a long.
//

#include <AverageFilter.h>

const int SENSOR_PIN = A0;

averageFilter<int, long> sensor(64);

void setup() {
    Serial.begin(115200);
    if (sensor.memoryError()) {
        Serial.println(F("Not enough memory: the filter passes samples through unchanged"));
    }
    sensor.setRounding(true);   // round to the nearest integer instead of truncating
}

void loop() {
    int raw = analogRead(SENSOR_PIN);
    int smooth = sensor.value(raw);

    Serial.print(F("raw="));
    Serial.print(raw);
    Serial.print(F(" smooth="));
    Serial.print(smooth);
    Serial.print(F(" samples="));
    Serial.print((unsigned long) sensor.count());
    Serial.print('/');
    Serial.println((unsigned long) sensor.window());

    delay(50);
}
