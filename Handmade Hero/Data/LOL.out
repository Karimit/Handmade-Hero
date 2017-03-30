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
			uint8 blue = (uint8)(x + xOffset);
			uint8 green = (uint8)(y + yOffset);
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
		if (tSine > (2.0f * PI32))
		{
			tSine -= (2.0f * PI32);
		}
	}
}

void GameUpdateAndRender(GameMemory* memory, GameOffscreenBuffer* buffer,
						 GameInput* input)
{	
	GameButtonState* terminator = &(input->Controllers[0].Terminator);
	Assert(terminator - &(input->Controllers[0].
		Buttons[0]) == ArrayCount(input->Controllers[0].Buttons));
	

	GameState* gameState = (GameState*)memory->PermenantStorage;
	Assert(sizeof(gameState) <= memory->PermenantStorageSize);
	char* fileName = __FILE__;
	DebugReadFileResult file = DEBUGPlatformReadEntireFile(fileName);
	if (file.Content)
	{
		DEBUGPlatformWriteFile("LOL.out", file.Content, file.ContentSize);
		DEBUGPlatformFreeFileMemory(file.Content);
	}

	if (!memory->IsInitialized)
	{
		gameState->ToneHz = 256;

		memory->IsInitialized = true;
	}

	for (int controllerIndex = 0; controllerIndex < ArrayCount(input->Controllers); controllerIndex++)
	{
		GameControllerInput* controller = GetController(input, controllerIndex);
		if (controller->IsAnalog)
		{
			//use analog movement tuning
			gameState->YOffset += (int)(4.0f*(controller->StickAverageX));
			gameState->ToneHz += 256 + (int)(128.0f*(controller->StickAverageY));
		}
		else
		{
			if (controller->MoveLeft.EnddedDown)
			{
				gameState->XOffset -= 1;
			}
			if (controller->MoveRight.EnddedDown)
			{
				gameState->XOffset += 1;
			}
		}
		if (controller->ActionUp.EnddedDown)
		{
			gameState->YOffset++;
			gameState->ToneHz += 64;
		}
		if (controller->ActionDown.EnddedDown)
		{
			gameState->ToneHz -= 64;
			if (gameState->ToneHz <= 0)
			{
				gameState->ToneHz = 1;
			}
		}
	}

	DrawWeirdGradient(buffer, gameState->XOffset, gameState->YOffset);
}

void GameGetSoundSamples(GameMemory* memory, GameSoundOutputBuffer* soundBuffer)
{
	GameState* gameState = (GameState*)memory->PermenantStorage;
	
	GameOutputSound(soundBuffer, gameState->ToneHz);
}