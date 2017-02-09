#pragma once
// Services that the game privde to the platform layer.
#include "util.h"

struct GameOffscreenBuffer
{
	int Width;
	int Height;
	void* Memory;
	int Pitch;
};

struct GameSoundOutputBuffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16* Samples;
};

struct GameButtonState
{
	int HalfTransitionCount;
	bool32 EnddedDown;
};

struct GameControllerInput {
	bool32 IsAnalog;

	real32 StartX;
	real32 StartY;
	real32 EndX;
	real32 EndY;

	real32 MinX;
	real32 MinY;
	real32 MaxX;
	real32 MaxY;

	union
	{
		GameButtonState Buttons[6];
		struct
		{
			GameButtonState Up;
			GameButtonState Down;
			GameButtonState Left;
			GameButtonState Right;
			GameButtonState LeftShoulder;
			GameButtonState RightShoulder;
		};
	};
};

struct GameInput
{
	GameControllerInput Controllers[4];
};

struct GameState
{
	int ToneHz;
	int XOffset;
	int YOffset;
};

struct GameMemory
{
	bool32 IsInitialized;
	uint64 PermenantStorageSize;
	void* PermenantStorage; //Required To be cleard to zeros on stratup

	uint64 TransientStorageSize;
	void* TransientStorage;
};

// Recieves controller/keyboard input, bitmap buffer and sound buffer to use, and time info.
void GameUpdateAndRender(GameMemory* memory, GameOffscreenBuffer* buffer, GameSoundOutputBuffer* soundBuffer, GameInput* input);

// Services that the platform layer privde to the game.