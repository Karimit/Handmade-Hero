#include "handmade.h"

void DrawWeirdGradient(GameOffscreenBuffer* buffer, int xOffset, int yOffset)
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

void GameUpdateAndRender(GameOffscreenBuffer* buffer, int xOffset, int yOffset)
{
	DrawWeirdGradient(buffer, xOffset, yOffset);
}