#pragma once

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#if SLOW_BUILD | DEBUG_BUILD
#define ASSERT(expression) if(!(expression)) *((int*)(0)) = 0;
#else
#define ASSERT(expression)
#endif

#if DEBUG_BUILD
#define DEBUG_LOG(...) fprintf(__VA_ARGS__)
#else
#define DEBUG_LOG(...) 
#endif