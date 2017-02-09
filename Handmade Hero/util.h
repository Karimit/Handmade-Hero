#pragma once
#include "stdint.h"
#include "math.h"

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

/*
	HANDMADE_INTERNAL: 0: build for public release
					   1: build for developer only
	HANDMADE_SLOW:	   0: No slow code Allowed!
					   1: Slow code welcomed.
*/
#if HANDMADE_SLOW
#define Assert(exp) if(!(exp)) {*(int*)0 = 0;}
#else
#define Assert(exp)
#endif

#define internal static
#define local_persist static
#define global_variable static

#define PI32 3.14159265359f

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)
