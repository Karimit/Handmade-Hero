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
// Recieves controller/keyboard input, bitmap buffer and sound buffer to use, and time info.
void GameUpdateAndRender(GameOffscreenBuffer* buffer, int xOffset, int yOffset,
						 GameSoundOutputBuffer* soundBuffer, int toneHz);

// Services that the platform layer privde to the game.