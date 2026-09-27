//
// Host unit tests for AverageFilter.h
// Build and run: make -C tests   (options: CXX=clang++ STD=c++17)
//
// Built with AddressSanitizer and UndefinedBehaviorSanitizer by the Makefile,
// so buffer overruns and double frees fail the run, not only wrong values.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <type_traits>

// Allocation-failure hook: the header's malloc calls go through test_malloc.
static bool g_failMalloc = false;
static void* test_malloc(size_t n) { return g_failMalloc ? NULL : malloc(n); }
#define malloc test_malloc
#include "AverageFilter.h"
#undef malloc

static int g_fail = 0, g_pass = 0;

#define CHECK(c) do { if (c) g_pass++; else { g_fail++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECK_EQ(a, b) do { long long _a = (long long)(a), _b = (long long)(b); \
    if (_a == _b) g_pass++; \
    else { g_fail++; printf("FAIL %s:%d: %s == %s (%lld vs %lld)\n", __FILE__, __LINE__, #a, #b, _a, _b); } } while (0)

static uint32_t g_rng = 12345;
static uint32_t rnd() { g_rng = g_rng * 1664525u + 1013904223u; return g_rng >> 8; }

// Reference model: mean of the last `win` samples, truncated like the filter (sum / count in type R)
template<typename R> struct Ref {
    R h[4096];
    long len = 0;
    int win;
    explicit Ref(int w) : win(w) {}
    void reset(int w) { len = 0; win = w; }
    R push(R s) {
        h[len % 4096] = s;
        len++;
        long n = len < win ? len : win;
        R sum = 0;
        for (long i = len - n; i < len; i++) sum += h[i % 4096];
        return sum / (R)n;
    }
};

// Not copyable: the filter owns its buffer
static_assert(!std::is_copy_constructible<averageFilter<long> >::value, "averageFilter must not be copyable");
static_assert(!std::is_copy_assignable<averageFilter<long> >::value, "averageFilter must not be copy-assignable");

static void testSetSamples() {
    {   // owned buffer grown past capacity: clamped, no overrun
        averageFilter<long> f(4); Ref<long> r(4);
        f.value(1);
        CHECK_EQ(f.setSamples(8), 4);
        for (int i = 0; i < 12; i++) CHECK_EQ(f.value(i), r.push(i));
    }
    {   // external storage: capacity is the constructor's count, nothing written past it
        long buf[10]; for (int i = 0; i < 10; i++) buf[i] = 1000;
        averageFilter<long> f(5, buf); Ref<long> r(5);
        CHECK_EQ(f.setSamples(10), 5);
        for (int i = 0; i < 12; i++) CHECK_EQ(f.value(i), r.push(i));
        CHECK_EQ(buf[5], 1000);
    }
    {   // shrink, then grow back within capacity: stale slots must not leak into the mean
        long buf[10];
        averageFilter<long> f(10, buf);
        for (int i = 0; i < 10; i++) f.value(1000);
        CHECK_EQ(f.setSamples(5), 5);
        CHECK_EQ(f.setSamples(10), 10);
        for (int i = 0; i < 10; i++) CHECK_EQ(f.value(1), 1);
    }
    {   // 0 and negative clamp to 1; history resets even with no samples
        averageFilter<long> f(4);
        CHECK_EQ(f.setSamples(0), 1);  CHECK_EQ(f.value(9), 9); CHECK_EQ(f.value(3), 3);
        CHECK_EQ(f.setSamples(-5), 1); CHECK_EQ(f.count(), 0);
    }
}

static void testConstructorCounts() {
    const int counts[] = { 0, -1, -32768 };
    for (int c : counts) {
        averageFilter<long> f(c);
        CHECK(!f.memoryError());
        CHECK_EQ(f.window(), 1);
        CHECK_EQ(f.value(5), 5); CHECK_EQ(f.value(-3), -3);
    }
    averageFilter<long> big(1000); Ref<long> r(1000);
    for (int i = 0; i < 1500; i++) CHECK_EQ(big.value(i % 97), r.push(i % 97));
}

static void testMemoryError() {
    {   g_failMalloc = true; averageFilter<long> f(8); g_failMalloc = false;
        CHECK(f.memoryError());
        CHECK_EQ(f.capacity(), 1);
        CHECK_EQ(f.setSamples(5), 1);
        CHECK_EQ(f.value(10), 10); CHECK_EQ(f.value(-4), -4);
        f.initialize(); CHECK(f.memoryError());
    }
    {   averageFilter<long> f(8, (long*)NULL);
        CHECK(f.memoryError());
        CHECK_EQ(f.value(7), 7); CHECK_EQ(f.value(2), 2);
    }
    {   g_failMalloc = true; averageFilter<float> f(8); g_failMalloc = false;
        CHECK(f.value(1.25f) == 1.25f); CHECK(f.value(-3.5f) == -3.5f);
    }
    {   long buf[4]; averageFilter<long> f(4, buf); CHECK(!f.memoryError()); }
}

static void testTotalType() {
    {   // 64 samples of 1023: overflows a 16-bit total, fits a 32-bit one
        averageFilter<int16_t, int32_t> f(64); int16_t v = 0;
        for (int i = 0; i < 64; i++) v = f.value(1023);
        CHECK_EQ(v, 1023);
    }
    {   // uint8_t percentages over 10 samples need a uint16_t total
        const int pct[] = { 30, 50, 100 };
        for (int k = 0; k < 3; k++) {
            uint8_t store[10], v = 0;
            averageFilter<uint8_t, uint16_t> f(10, store);
            for (int i = 0; i < 25; i++) v = f.value((uint8_t)pct[k]);
            CHECK_EQ(v, pct[k]);
        }
    }
    {   averageFilter<int8_t, int16_t> f(4); Ref<long> r(4);
        for (int i = 0; i < 200; i++) { int s = (int)(rnd() % 256) - 128; CHECK_EQ(f.value((int8_t)s), r.push(s)); }
    }
}

static void testRandomizedAgainstReference() {
    {   // garbage-filled storage, random initialize() and setSamples() calls
        long buf[32]; for (int i = 0; i < 32; i++) buf[i] = 0x55AA55AAL;
        averageFilter<long> f(32, buf); Ref<long> r(32); long mismatches = 0;
        for (int i = 0; i < 20000; i++) {
            uint32_t op = rnd() % 1000;
            if (op == 0) { f.initialize(); r.reset(r.win); }
            else if (op == 1) {
                int w = 1 + (int)(rnd() % 40);
                int applied = (int)f.setSamples(w);
                CHECK_EQ(applied, w > 32 ? 32 : w);
                r.reset(applied);
            }
            long s = (long)(rnd() % 20001) - 10000;
            if (f.value(s) != r.push(s)) mismatches++;
        }
        CHECK_EQ(mismatches, 0);
    }
    {   // initialize() and setSamples() do not write to caller storage
        long buf[8]; for (int i = 0; i < 8; i++) buf[i] = 1234;
        averageFilter<long> f(8, buf);
        f.value(5); f.initialize(); f.setSamples(4);
        CHECK_EQ(buf[0], 5); CHECK_EQ(buf[1], 1234); CHECK_EQ(buf[7], 1234);
    }
    {   averageFilter<unsigned long> f(5); Ref<unsigned long> r(5); long mismatches = 0;
        for (int i = 0; i < 5000; i++) { unsigned long s = rnd() % 1000000; if (f.value(s) != r.push(s)) mismatches++; }
        CHECK_EQ(mismatches, 0);
    }
}

template<typename F, typename Flt> static double residual(Flt& f, long n, int tail) {
    g_rng = 777;
    for (long i = 0; i < n; i++) f.value((F)((rnd() % 1000000) / 1000.0));   // fractional samples in [0, 1000)
    F v = 0;
    for (int i = 0; i < tail; i++) v = f.value((F)0);
    return fabs((double)v);
}

static void testFloatDrift() {
    {   averageFilter<float> aligned(10), unaligned(10);
        CHECK(residual<float>(aligned, 1000000, 10) == 0.0);    // zeros fill a whole aligned window
        CHECK(residual<float>(unaligned, 1000003, 10) < 1e-3);
    }
    {   averageFilter<double> d(10);
        CHECK(residual<double>(d, 1000003, 10) < 1e-9);
    }
    {   averageFilter<float> f(16); Ref<double> r(16); double maxErr = 0;
        for (int i = 0; i < 3000; i++) {
            if (i == 1000) { CHECK_EQ(f.setSamples(5), 5); r.reset(5); }
            if (i == 2000) { CHECK_EQ(f.setSamples(16), 16); r.reset(16); }
            float s = (float)((rnd() % 200000) / 100.0 - 1000.0);
            double e = fabs((double)f.value(s) - r.push(s));
            if (e > maxErr) maxErr = e;
        }
        CHECK(maxErr < 0.01);
    }
}

template<typename T, typename S> static T feed(averageFilter<T, S>& f, const long* v, int n) {
    T r = 0;
    for (int i = 0; i < n; i++) r = f.value((T)v[i]);
    return r;
}

static void testRounding() {
    const long a[] = { 1, 2 }, b[] = { -1, -2 }, c[] = { 1, 1, 2 }, d[] = { -1, -1, -2 }, e[] = { 5, 6, 6, 6 };
    {   averageFilter<long> f(8);    // default: truncation toward zero
        CHECK_EQ(feed(f, a, 2), 1); f.initialize(); CHECK_EQ(feed(f, b, 2), -1);
    }
    {   averageFilter<long> f(8); f.setRounding(true);
        CHECK_EQ(feed(f, a, 2), 2);  f.initialize();
        CHECK_EQ(feed(f, b, 2), -2); f.initialize();
        CHECK_EQ(feed(f, c, 3), 1);  f.initialize();
        CHECK_EQ(feed(f, d, 3), -1); f.initialize();
        CHECK_EQ(feed(f, e, 4), 6);
    }
    {   averageFilter<int, long> f(7); f.setRounding(true); long h[7]; int n = 0; long mismatches = 0;
        for (int k = 0; k < 20000; k++) {
            long s = (long)(rnd() % 20001) - 10000;
            h[k % 7] = s; if (n < 7) n++;
            long sum = 0; for (int i = 0; i < n; i++) sum += h[i];
            if ((long)f.value((int)s) != lround((double)sum / n)) mismatches++;
        }
        CHECK_EQ(mismatches, 0);
    }
    {   averageFilter<float> f(2); f.setRounding(true);    // no effect on floating point
        CHECK(f.value(1.0f) == 1.0f); CHECK(f.value(2.0f) == 1.5f);
    }
}

static long readAll(const averageFilter<long>& f) {   // every getter is callable on a const filter
    return (long)f.currentValue() + (long)f.window() + (long)f.capacity() + (long)f.count()
         + f.full() + (long)f.samples() + f.memoryError();
}

static void testGetters() {
    averageFilter<long> f(8);
    CHECK_EQ(f.window(), 8); CHECK_EQ(f.capacity(), 8); CHECK_EQ(f.count(), 0); CHECK(!f.full());
    for (int i = 1; i <= 7; i++) { f.value(i); CHECK_EQ(f.count(), i); CHECK_EQ(f.samples(), i); CHECK(!f.full()); }
    f.value(8); CHECK(f.full());
    f.value(9); CHECK(f.full()); CHECK_EQ(f.count(), 8);
    CHECK_EQ(f.setSamples(3), 3); CHECK_EQ(f.capacity(), 8); CHECK(!f.full());
    CHECK_EQ(readAll(f), 0 + 3 + 8 + 0 + 0 + 0 + 0);
}

int main() {
    testSetSamples();
    testConstructorCounts();
    testMemoryError();
    testTotalType();
    testRandomizedAgainstReference();
    testFloatDrift();
    testRounding();
    testGetters();
    printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
