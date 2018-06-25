#pragma once
#include "project_types.h"
#include <math.h>

inline int32 roundReal32ToInt32(real32 val)
{
    int32 result = (int32)(val + 0.5f);
    //int32 result = (int32)(roundf(val));
    return result;
}

inline int32 truncateReal32ToInt32(real32 val)
{
    int32 result = (int32)(val);
    return result;
}

#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif
