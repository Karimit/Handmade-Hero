#pragma once
struct Win32OffscreenBuffer
{
	int Width;
	int Height;
	BITMAPINFO Info;
	void* Memory;
	int Pitch;
};

struct Win32SoundOutput
{
	int SamplesPerSecond;
	int BytesPerSample;
	uint32 RunningSampleIndex;
	int SoundBufferSize;
	real32 TSine;
	int LatencySampleCount;
};

struct Win32WindowDimension
{
	int Width;
	int Height;
};
