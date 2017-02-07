#include "handmade.h"

internal void DrawWeirdGradient(GameOffscreenBuffer* buffer, int xOffset, int yOffset)
{
	uint32* pixel = (uint32*)buffer->Memory;
	uint8* row = (uint8*)buffer->Memory;
	for (int y = 0; y < buffer->Height; y++)
	{
		pixel = (uint32*)row;
		for (int x = 0; x < buffer->Width; x++)
		{
			uint8 blue = x + xOffset;
			uint8 green = y + yOffset;
			*pixel++ = (uint32)((green << 8) | blue);
		}
		row += buffer->Pitch;
	}
}

internal void GameOutputSound (GameSoundOutputBuffer* soundBuffer, int toneHz)
{
	int16* sampleOut = (int16*)soundBuffer->Samples;
	local_persist real32 tSine;
	int wavePeriod = soundBuffer->SamplesPerSecond / toneHz;
	int16 toneVolume = 1200;

	for (int sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; sampleIndex++)
	{
		real32 sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;
		tSine += (2.0f * PI32) / (real32)wavePeriod;
	}
}

void GameUpdateAndRender(GameOffscreenBuffer* buffer, int xOffset, int yOffset,
						 GameSoundOutputBuffer* soundBuffer, int toneHz)
{
	GameOutputSound(soundBuffer, toneHz);
	DrawWeirdGradient(buffer, xOffset, yOffset);
}