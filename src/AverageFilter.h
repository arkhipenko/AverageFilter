//
// Digital implementation of an average filter
// with variable number of samples
//
// T: sample type. Must be a numeric type: the buffer comes from malloc,
// which does not run constructors.
// S: running total type, defaults to T.
// S must hold window size times the largest sample,
// for example averageFilter<int, long> or averageFilter<uint8_t, uint16_t>.
//
// External storage must hold at least aSamples elements and outlive the filter.
//

#ifndef AVERAGEFILTER_H
#define AVERAGEFILTER_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

template<typename T, typename S = T>
class averageFilter {
    public:
        averageFilter(int aSamples);
        averageFilter(int aSamples, T* aStorage);
        ~averageFilter();

        // Not copyable: the filter owns its buffer.
        averageFilter(const averageFilter&) = delete;
        averageFilter& operator=(const averageFilter&) = delete;

        void    initialize();
        T       value(T aSample);
        size_t  setSamples( int aSamples );
        void    setRounding( bool aRounding ) { iRounding = aRounding; }   // integer means: round half away from zero (default: truncate)

        T       currentValue() const { return iCV; }
        size_t  window() const { return iSamples; }         // configured window size
        size_t  capacity() const { return iCapacity; }      // largest window setSamples() accepts
        size_t  count() const { return iCount; }            // samples currently held
        bool    full() const { return iCount == iSamples; }
        size_t  samples() const { return iCount; }          // deprecated: same as count(), not the window size
        bool    memoryError() const { return iMemoryError; }

    private:
        static size_t toCount( int aSamples ) { return aSamples < 1 ? 1 : (size_t) aSamples; }   // 0 or negative means 1
        void    attachBuffer( T* aBuffer, size_t aSamples, bool aOwned );

        T       *iReadings;
        bool    iStorageExternal = false;
        bool    iMemoryError = false;       // set when the filter fell back to 1-sample pass-through
        bool    iRounding = false;
        S       iTotal;
        T       iCV;
        size_t  iIndex;
        size_t  iCount;
        size_t  iSamples;
        size_t  iCapacity;                  // largest window the buffer can hold
};



template<typename T, typename S>
averageFilter<T, S>::averageFilter(int aSamples) {
    size_t n = toCount(aSamples);
    T* buffer = NULL;
// a byte size that does not fit in size_t (16-bit on AVR) counts as an allocation error
    if ( n <= ((size_t) -1) / sizeof (T) ) buffer = (T *) malloc (sizeof (T) * n);
    attachBuffer(buffer, n, true);
}

template<typename T, typename S>
averageFilter<T, S>::averageFilter(int aSamples, T* aStorage) {
    attachBuffer(aStorage, toCount(aSamples), false);
}

// Shared constructor setup. A NULL buffer (failed allocation or NULL storage)
// is a memory error: the filter becomes a 1-sample pass-through that uses iCV as its buffer.
template<typename T, typename S>
void averageFilter<T, S>::attachBuffer(T* aBuffer, size_t aSamples, bool aOwned) {
    if ( aBuffer == NULL ) {
        aBuffer = &iCV;
        aSamples = 1;
        aOwned = false;
        iMemoryError = true;
    }
    iReadings = aBuffer;
    iStorageExternal = !aOwned;
    iSamples = aSamples;
    iCapacity = aSamples;
    initialize();
}

// Window is clamped to [1, capacity]; always resets history.
// Returns the window size actually applied.
template<typename T, typename S>
size_t averageFilter<T, S>::setSamples(int aSamples) {
    size_t n = toCount(aSamples);
    iSamples = n > iCapacity ? iCapacity : n;
    initialize();
    return iSamples;
}

template<typename T, typename S>
averageFilter<T, S>::~averageFilter() {
    if ( !iStorageExternal ) free (iReadings);
}

template<typename T, typename S>
void averageFilter<T, S>::initialize() {
    iIndex = 0;
    iTotal = 0;
    iCount = 0;
    iCV = 0;
}

template<typename T, typename S>
T   averageFilter<T, S>::value(T aSample) {
// while the window fills, the slot at the write index is unused: subtract only when full
    if ( iCount == iSamples ) iTotal -= iReadings[iIndex]; else iCount++;
    iReadings[iIndex] = aSample;
    iTotal += aSample;
    if ( ++iIndex >= iSamples ) iIndex = 0;
// floating-point total: recompute once per full window to stop rounding drift.
// (S)0.5 != (S)0 is a compile-time constant, so integer builds carry no extra code.
    if ( (S)0.5 != (S)0 && iIndex == 0 && iCount == iSamples ) {
        iTotal = 0;
        for (size_t i = 0; i < iSamples; i++) iTotal += iReadings[i];
    }
// integer total with rounding on: round half away from zero instead of truncating
    if ( iRounding && (S)0.5 == (S)0 )
        iCV = (T)((iTotal >= 0 ? iTotal + (S)iCount / 2 : iTotal - (S)iCount / 2) / (S)iCount);
    else
        iCV = (T)(iTotal / (S)iCount);
    return iCV;
}

#endif  // AVERAGEFILTER_H
