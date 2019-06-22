#include "project_types.h"
#include <stdlib.h>
#include <math.h>
#define PI 3.14159265358979323846f
#define DEG_TO_RAD (PI/180.0f)
#define RAD_TO_DEG (180.0f/PI)

#pragma pack(push, 1)
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
    real32 w;
    real32 h;
};

struct v2i32
{
    int32 x;
    int32 y;
};


struct v2u32
{
    uint32 x;
    uint32 y;
};

#pragma pack(pop)

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


v2 operator/(v2 a, real32 b)
{
    v2 result;
    result.x = a.x/b;
    result.y = a.y/b;
    
    return result;
}


v2& operator/=(v2 &a, real32 b)
{
    a = a/b;
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


v3 operator/(v3 a, real32 b)
{
    v3 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    
    return result;
}


v3& operator/=(v3 &a, real32 b)
{
    a = a / b;
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


real32 SquaredMagnitude(v2 v)
{
    real32 result;
    result = (v.x * v.x) + (v.y * v.y);
    return result;
}

real32 Magnitude(v3 v)
{
    real32 result;
    result = SquareRoot(SquaredMagnitude(v));
    return result;
}

real32 Magnitude(v2 v)
{
    real32 result;
    result = SquareRoot(SquaredMagnitude(v));
    return result;
}

v2 Normalize(v2 v)
{
    v2 result = {};
    real32 mag = Magnitude(v);
    result = v/mag;
    return result;
}


v3 Normalize(v3 v)
{
    v3 result = {};
    real32 mag = Magnitude(v);
    result = v/mag;
    return result;
}


real32 Rand(int32 min, int32 max)
{
    return (real32)(min + rand() % (max - min));
}

void RotationAboutZAxis(mat4 *mat, real32 degrees)
{
    mat4 clearMat = {};
    *mat = clearMat;
    
    real32 cosine = Cosine(DEG_TO_RAD * degrees);
    real32 sine = Sine(DEG_TO_RAD * degrees);
    mat->data[0] = cosine;
    mat->data[1] = -sine;
    mat->data[4] = sine;
    mat->data[5] = cosine;
    mat->data[10] = 1.0f;
    mat->data[15] = 1.0f;
}

void Identity(mat4 *mat)
{
    mat4 clearMat = {};
    *mat = clearMat;
    
    mat->data[0] = 1.0f;
    mat->data[5] = 1.0f;
    mat->data[10] = 1.0f;
    mat->data[15] = 1.0f;
}

void TranslationMat(mat4 *mat, v3 t)
{
    Identity(mat);
    mat->data[3] = t.x;
    mat->data[7] = t.y;
    mat->data[11] = t.z;
}


real32 RemapRange(real32 initialMin, real32 initialMax, real32 newMin, real32 newMax, real32 initialValue)
{
    real32 result;
    result = (((initialValue - initialMin)/(initialMax - initialMin)) * (newMax - newMin)) + newMin;
    return result;
}


bool CircleWithCircleCollisionTest(v2 c1, real32 r1, v2 c2, real32 r2)
{
    real32 squaredDistance = SquaredMagnitude(c2 - c1);
    real32 collisionDistance = r1 + r2;
    real32 squaredCollisionDistance = collisionDistance * collisionDistance;
    return (squaredDistance <= squaredCollisionDistance);
}


inline static int64 CeilReal32ToInt64(real32 real32Value)
{
    // TODO(KARAN): roundf is too slow, replace it with something faster
    
    // Doing roundf for every pixel tanked the 
    // frame rate from 30ms to 500ms.
    
    int64 result = (int64)(real32Value + 0.5f);
    //int32 result = (int32)(ceilf(real32Value));
    return result;
}


inline static uint64 CeilReal32ToUint64(real32 real32Value)
{
    // TODO(KARAN): roundf is too slow, replace it with something faster
    
    // Doing roundf for every pixel tanked the 
    // frame rate from 30ms to 500ms.
    
    uint64 result = (uint64)(real32Value + 0.5f);
    //int32 result = (int32)(ceilf(real32Value));
    return result;
}


inline static int32 CeilReal32ToInt32(real32 real32Value)
{
    // TODO(KARAN): roundf is too slow, replace it with something faster
    
    // Doing roundf for every pixel tanked the 
    // frame rate from 30ms to 500ms.
    
    int32 result = (int32)(real32Value + 0.5f);
    //int32 result = (int32)(ceilf(real32Value));
    return result;
}

real32 Floor(real32 val)
{
    return floorf(val);
}


inline static uint32 CeilReal32ToUint32(real32 real32Value)
{
    // TODO(KARAN): roundf is too slow, replace it with something faster
    
    // Doing roundf for every pixel tanked the 
    // frame rate from 30ms to 500ms.
    
    uint32 result = (uint32)(real32Value + 0.5f);
    //int32 result = (int32)(ceilf(real32Value));
    return result;
}

void WrapAroundIfOutOfBounds(v2 *point, v4 rect)
{
    if(point->x < rect.x)
    {
        real32 diff = rect.x - point->x;
        point->x = (rect.x + rect.w) - diff;
    }
    else if(point->x >= (rect.x + rect.w))
    {
        real32 diff = point->x - (rect.x + rect.w);
        point->x = rect.x + diff;
    }
    
    if(point->y < rect.y)
    {
        real32 diff = rect.y - point->y;
        point->y = (rect.y + rect.h) - diff;
    }
    else if(point->y >= (rect.y + rect.h))
    {
        real32 diff = point->y - (rect.y + rect.h);
        point->y = rect.y + diff;
    }
}

bool IsPointInRect(v4 rect, v2 point)
{
    bool result;
    real32 minX = rect.x;
    real32 minY = rect.y;
    real32 maxX = minX + rect.w - 1.0f;
    real32 maxY = minY + rect.h - 1.0f;
    
    result = ((point.x >= minX) && (point.x <= maxX) &&
              (point.y >= minY) && (point.y <= maxY));
    
    return result;
}

inline int32
RoundReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)roundf(Real32);
    return(Result);
}

inline uint32
RoundReal32ToUint32(real32 Real32)
{
    uint32 Result = (uint32)roundf(Real32);
    return(Result);
}
