#include "project_types.h"
#include <math.h>
#define PI 3.14159265358979323846f
#define DEG_TO_RAD (PI/180.0f)
#define RAD_TO_DEG (180.0f/PI)

struct v2
{
    real32 x;
    real32 y;
};

struct v3
{
    real32 x;
    real32 y;
    real32 z;
};

struct v4
{
    real32 x;
    real32 y;
    real32 z;
    real32 w;
};

struct mat4
{
    real32 data[16];
};


inline real32 Sine(real32 radians)
{
    real32 result = sinf(radians);
    return result;
}

inline real32 Cosine(real32 radians)
{
    real32 result = cosf(radians);
    return result;
}


v2 operator+(v2 a, v2 b)
{
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    
    return result;
}


v2 operator-(v2 a, v2 b)
{
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    
    return result;
}


v2 operator-(v2 a)
{
    v2 result;
    result.x = -a.x;
    result.y = -a.y;
    
    return result;
}


v2 operator*(v2 a, real32 b)
{
    v2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    
    return result;
}


v2& operator*=(v2 &a, real32 b)
{
    a = a * b;
    return a;
}


v2& operator+=(v2 &a, v2 b)
{
    a = a + b;
    return a;
}


v2& operator-=(v2 &a, v2 b)
{
    a = a - b;
    return a;
}

v3 operator+(v3 a, v3 b)
{
    v3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    
    return result;
}


v3 operator-(v3 a, v3 b)
{
    v3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    
    return result;
}


v3 operator-(v3 a)
{
    v3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    
    return result;
}


v3 operator*(v3 a, real32 b)
{
    v3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    
    return result;
}


v3& operator*=(v3 &a, real32 b)
{
    a = a * b;
    return a;
}


v3& operator+=(v3 &a, v3 b)
{
    a = a + b;
    return a;
}


v3& operator-=(v3 &a, v3 b)
{
    a = a - b;
    return a;
}

real32 SquareRoot(real32 value)
{
    real32 result;
    result = sqrtf(value);
    return result;
}

real32 SquaredMagnitude(v3 v)
{
    real32 result;
    result = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
    return result;
}

real32 Magnitude(v3 v)
{
    real32 result;
    result = SquareRoot(SquaredMagnitude(v));
    return result;
}