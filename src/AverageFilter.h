//
// Digital implementation of an average filter
// with variable number of samples
//

#include <Arduino.h>

#ifndef _AVERAGEFILTER_H
#define _AVERAGEFILTER_H

template<typename T>
class averageFilter {
    public:
        averageFilter(int aSamples);
        ~averageFilter();
        
        void    initialize();
        T       value(T aSample);
        T       currentValue();
        int     samples() { return iCount; }
        
    private:
        T       *iReadings;
        T       iTotal;
        int     iIndex;
        int     iCount;
        int     iSamples;
};

template<typename T>
averageFilter<T>::averageFilter(int aSamples) {
    iSamples = aSamples; 
    iReadings = (T *) malloc (sizeof (T) * aSamples);
// if there is a memory allocation error.
//    if (iReadings == NULL)
//        exit ("averageFilter: insufficient memory.");
    initialize();
}

template<typename T>
averageFilter<T>::~averageFilter() {
    free (iReadings);
    iReadings = NULL;
    iIndex = 0;
    iTotal = 0;
    iCount = 0;
}

template<typename T>
void averageFilter<T>::initialize() {
    iIndex = 0;
    iTotal = 0;
    iCount = 0;
    for (int i=0; i<iSamples; i++) iReadings[i]=0;
}

template<typename T>
T   averageFilter<T>::value(T aSample) {
    iTotal -= iReadings[iIndex];
    iReadings[iIndex] = aSample;
    iTotal += aSample;
    if (++iIndex >= iSamples) iIndex = 0;
    if (++iCount > iSamples) iCount = iSamples;
    return (iTotal/iCount);
}

template<typename T>
T   averageFilter<T>::currentValue() {
    return ( iCount == 0 ? 0 : iTotal/iCount );
}


#endif  // _AVERAGEFILTER_H

