//
// BatteryLevel: average a battery percentage in storage you provide.
//
// Shows external storage (no heap use), a narrow sample type with a wider
// total, full() to wait for a complete window, and setSamples() to change
// the window at run time within the storage size.
//

#include <AverageFilter.h>

const int BATTERY_PIN = A0;
const int WINDOW_MAX = 20;

uint8_t storage[WINDOW_MAX];
averageFilter<uint8_t, uint16_t> battery(WINDOW_MAX, storage);   // 20 x 100 = 2000 fits a uint16_t

// Scale a 10-bit ADC reading to 0-100 percent.
// Boards with a 12-bit ADC (for example ESP32) read higher; the result is clamped to 100.
uint8_t readPercent() {
    long raw = analogRead(BATTERY_PIN);
    return (uint8_t) constrain(map(raw, 0, 1023, 0, 100), 0, 100);
}

void setup() {
    Serial.begin(115200);
    // Start with a short window for a quick first reading.
    // setSamples() never exceeds the storage size and returns the window applied.
    size_t applied = battery.setSamples(5);
    Serial.print(F("window: "));
    Serial.println((unsigned long) applied);
}

void loop() {
    uint8_t level = battery.value(readPercent());

    if (battery.full()) {
        Serial.print(F("battery: "));
        Serial.print(level);
        Serial.println('%');

        // After the first reading, switch to the full window. History restarts.
        if (battery.window() < battery.capacity()) {
            battery.setSamples((int) battery.capacity());
            Serial.print(F("window: "));
            Serial.println((unsigned long) battery.window());
        }
    }

    delay(200);
}
