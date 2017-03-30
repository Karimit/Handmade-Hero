#pragma once
struct Win32OffscreenBuffer
{
	int Width;
	int Height;
	BITMAPINFO Info;
	void* Memory;
	int Pitch;
	int BytesPerPixel;
};

struct Win32SoundOutput
{
	int SamplesPerSecond;
	int BytesPerSample;
	uint32 RunningSampleIndex;
	DWORD SoundBufferSize;
	real32 TSine;
	DWORD SafetyBytes;
};

struct Win32WindowDimension
{
	int Width;
	int Height;
};

struct Win32DebugTimeMarker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;

	DWORD OutputLocation; //bytetolock
	DWORD OutputByteCount;

	DWORD ExpectedFlipPlayCursor;
	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};
