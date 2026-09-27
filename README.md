# Average Filter

### Digital implementation of an average filter
[![arduino-library-badge](https://www.ardu-badge.com/badge/AverageFilter.svg?)](https://www.ardu-badge.com/AverageFilter)

##### With variable number of tracked values and externally provided storage

##### Version 1.1.0

A moving average over the last N samples of any numeric type. Each new sample updates the average in constant time.

###### Usage

```cpp
#include <AverageFilter.h>

averageFilter<int, long> adc(16);                      // 16 samples, buffer from the heap, long total
uint8_t store[10];
averageFilter<uint8_t, uint16_t> battery(10, store);   // 10 samples in your own array

void setup() {
    adc.setRounding(true);                             // optional: round instead of truncate
}

void loop() {
    int level = adc.value(analogRead(A0));             // add a sample, get the new average
    if (adc.full()) {
        // the average now covers a full window of 16 samples
    }
}
```

###### Template parameters

- `T`: sample type. Must be a numeric type, because the buffer comes from `malloc`, which does not run constructors.
- `S`: running total type, defaults to `T`. It must hold the window size times the largest sample. Example: 64 samples of `analogRead()` (up to 1023) need 65472, which does not fit a 16-bit `int`, so use `averageFilter<int, long>`. With rounding on, `S` must also hold half the window size on top of that.

###### API

- `averageFilter(int samples)`: window of `samples` (0 or negative means 1), buffer from `malloc`. If the allocation fails, the filter passes samples through unchanged and `memoryError()` returns true.
- `averageFilter(int samples, T* storage)`: uses your array, which must hold at least `samples` elements and outlive the filter. The filter never frees it. NULL storage gives the pass-through filter.
- `T value(T sample)`: adds a sample and returns the average of the samples held.
- `T currentValue()`: the last average, without adding a sample.
- `void initialize()`: clears the history. The buffer contents are not touched.
- `size_t setSamples(int samples)`: changes the window size, up to `capacity()`, and clears the history. Returns the window size applied.
- `void setRounding(bool on)`: integer averages round half away from zero instead of truncating toward zero. Off by default. No effect when `S` is a floating-point type.
- `size_t window()`: the window size.
- `size_t capacity()`: the largest window the buffer can hold.
- `size_t count()`: the number of samples held, at most the window size.
- `bool full()`: true when a full window of samples is held.
- `bool memoryError()`: true if the filter fell back to passing samples through.
- `size_t samples()`: deprecated, same as `count()`. It is not the window size.

Filters cannot be copied. Pass them by reference or pointer, or hold them as members.
With a floating-point total, the total is recomputed once per window, so rounding error does not build up over long runs.

###### Changelog:

v1.0.0:

- 2015-11-18 - initial release

v1.0.1:

- 2021-02-02 - header comment and formatting

v1.0.2:

- 2025-11-14 - bug: potential for freeing static memory fixed

v1.1.0:

- 2026-09-27 - bug: setSamples() wrote past the buffer when the window grew. The window is now limited to capacity().
- 2026-09-27 - bug: copying a filter freed its buffer twice. Copying is now a compile error.
- 2026-09-27 - bug: a sample count of 0 wrote past a zero-size buffer, and on AVR a very large count could allocate a buffer that was too small.
- 2026-09-27 - bug: memoryError() never reported a failure.
- 2026-09-27 - bug: the running total overflowed for narrow types. New template parameter `S` sets the total type.
- 2026-09-27 - bug: floating-point averages drifted over long runs.
- 2026-09-27 - new: window(), capacity(), count(), full() and setRounding().
- 2026-09-27 - change: setSamples() returns the window size applied and always clears the history. initialize() no longer clears the buffer. Counts are `size_t`.
- 2026-09-27 - change: the header no longer includes Arduino.h. A .cpp file that relied on it needs its own `#include <Arduino.h>`. Sketches are not affected.
- 2026-09-27 - deprecated: samples(). Use count().
