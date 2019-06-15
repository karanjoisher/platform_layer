#pragma once

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#if DEBUG_BUILD
#define ASSERT(expression, assertMessage) if(!(expression)) fprintf(stderr, "ASSERTION FAILED: %s [LINE: %d, FUNCTION:%s, FILE:%s]\n",assertMessage, __LINE__, __func__, __FILE__), *((int*)(0)) = 0;
#else
#define ASSERT(expression, assertMessage)
#endif

#if DEBUG_BUILD
#define DEBUG_LOG(...) fprintf(stdout, __VA_ARGS__)
#else
#define DEBUG_LOG(...) 
#endif


#if DEBUG_BUILD
#define DEBUG_ERROR(...) fprintf(stderr, "ERROR: "),fprintf(stderr, __VA_ARGS__), fprintf(stderr, " [LINE: %d, FUNCTION:%s, FILE:%s]\n",__LINE__, __func__, __FILE__)
#else
#define DEBUG_ERROR(...) 
#endif


#define KILOBYTES(Value) ((Value)*1024LL)
#define MEGABYTES(Value) (KILOBYTES(Value)*1024LL)
#define GIGABYTES(Value) (MEGABYTES(Value)*1024LL)
#define TERABYTES(Value) (GIGABYTES(Value)*1024LL)

bool AreStringsSame(char *str1, char *str2)
{
    bool result = true;
    int i = 0;
    
    while(result)
    {
        result = str1[i] == str2[i];
        if(str1[i] == 0 || str2[i] == 0)
        {
            break;
        }
        ++i;
    }
    
    return result;
}


void Copy(char *destination, char*source, int sourceLength)
{
    for(int i = 0; i < sourceLength; i++)
    {
        *destination++ = *source++;
    }
}


int GetElementIndex(int *array, int arrayLength, int element)
{
    int result = -1;
    for(int i = 0; i < arrayLength; i++)
    {
        if(array[i] == element)
        {
            result = i;
            break;
        }
    }
    
    return result;
}

void ClearArray(char *array, int size, char clearValue = 0)
{
    while(size--)
    {
        *array++ = clearValue;
    }
}

bool EndsWith(char *source, char *endStr)
{
    int sourceLength = 0;
    for(;source[sourceLength] != 0; sourceLength++);
    
    int endStrLength = 0;
    for(;endStr[endStrLength] != 0; endStrLength++);
    
    int sourceIndex = sourceLength - endStrLength;
    bool areSame = true;
    for(int i = 0; i < endStrLength; i++)
    {
        areSame = source[sourceIndex + i] == endStr[i];
        if(!areSame)
        {
            return false;
        }
    }
    return true;
}

void ConcatenateStrings(char *str1, char *str2, char *dest)
{
    int destIndex = 0;
    for(int i = 0; str1[i] != 0; i++, destIndex++)
    {
        dest[destIndex] = str1[i]; 
    }
    
    for(int i = 0; str2[i] != 0; i++, destIndex++)
    {
        dest[destIndex] = str2[i]; 
    }
    
    dest[destIndex + 1] = 0;
}

int GetStrLength(char *str)
{
    int length = 0;
    for(;str[length] != 0; length++);
    return length;
}