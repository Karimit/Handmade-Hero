#pragma once
#include "util.h"

// Services that the platform layer provides to the game.
#if HANDMADE_INTERNAL
struct DebugReadFileResult
{
	uint32 ContentSize;
	void* Content;
};
DebugReadFileResult DEBUGPlatformReadEntireFile(char* fileName);
void DEBUGPlatformFreeFileMemory(void* memory);

bool32 DEBUGPlatformWriteFile(char* fileName, void* memory, uint32 fileSize);
#endif // HANDMADE_INTERNAL


// Services that the game provides to the platform layer.

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

struct GameControllerInput
{
	bool32 IsConnected;
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;

	union
	{
		GameButtonState Buttons[12];
		struct
		{
			GameButtonState MoveUp;
			GameButtonState MoveDown;
			GameButtonState MoveLeft;
			GameButtonState MoveRight;

			GameButtonState ActionUp;
			GameButtonState ActionDown;
			GameButtonState ActionLeft;
			GameButtonState ActionRight;

			GameButtonState LeftShoulder;
			GameButtonState RightShoulder;

			GameButtonState Back;
			GameButtonState Start;

			// To ensure the struct has as many members as the Buttons array.
			GameButtonState Terminator;
		};
	};
};

struct GameInput
{
	GameControllerInput Controllers[5];
};

inline GameControllerInput* GetController(GameInput* input, int unsigned controlleIndex)
{
	Assert(controlleIndex < ArrayCount(input->Controllers));
	return &input->Controllers[controlleIndex];
}

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
