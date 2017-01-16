#include "windows.h"
#include "stdint.h"
#include "xinput.h"
#include "dsound.h"
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

#define internal static
#define local_persist static
#define global_variable static

#define PI32 3.14159265359f

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
	int ToneHz;
	int WavePeriod;
	int SoundBufferSize;
	int ToneVolume;
	real32 TSine;
	int LatencySampleCount;
	void UpdateToneHz(int toneHz)
	{
		ToneHz = toneHz;
		WavePeriod = SamplesPerSecond / ToneHz;
	}
};

global_variable bool GlobalRunning;
global_variable Win32OffscreenBuffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer; // The actual sound buffer that we will write to.
global_variable Win32SoundOutput GlobalSoundOutput;

struct Win32WindowDimension
{
	int Width;
	int Height;
};

//Support for xInputGetState and xInputSetState without having to reference xinput.dll because
// many windows installations (especially win 7) doesnt have it by default
//We are creating a function pointer to point to xInputGetState and xInputSetState, and having
// these func pointers point to a stub func in case xinput1_4.dll doesnt exist and using the
// game pad is not possible. Otherwise, if we actually referenced that lib and it didnt exist,
// then the game will crash even though want the user t be able to play using the keyboard.
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(X_Input_Get_State);
#define XInputGetState XInputGetState_ //to not collide with the original XInputGetState in xinput.dll, this way when we refer to XInputGetState
									   // we are referencing our own XInputGetState_

X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable X_Input_Get_State* XInputGetState_ = XInputGetStateStub;

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(X_Input_Set_State);
#define XInputSetState XInputSetState_

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable X_Input_Set_State* XInputSetState_ = XInputSetStateStub;

internal void Win32LoadXInput()
{
	HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!xInputLibrary)
	{
		HMODULE xInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}
	if (!xInputLibrary)
	{
		HMODULE xInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if (xInputLibrary)
	{
		XInputGetState = (X_Input_Get_State*) GetProcAddress(xInputLibrary, "XInputGetState");
		if (!XInputGetState)
			XInputGetState = XInputGetStateStub;
		XInputSetState = (X_Input_Set_State*) GetProcAddress(xInputLibrary, "XInputSetState");
		if (!XInputSetState)
			XInputSetState = XInputSetStateStub;
	}
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUND* ppDS, _Pre_null_ LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(Direct_Sound_Create);

internal void Win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
	// Load the Libray: while allowing the game to run if we were not able to load it.
	HMODULE dSoundLibrary = LoadLibrary("dsound.dll");
	if (dSoundLibrary)
	{
		// Get a DirectSound object
		Direct_Sound_Create* directSoundCreate = (Direct_Sound_Create*)GetProcAddress(dSoundLibrary, "DirectSoundCreate");
		LPDIRECTSOUND directSound;
		if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat = {};
			waveFormat.cbSize = 0;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = samplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

			if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				// Create a primary buffer (it just sets the mode of the sound card)
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
				{
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
					{
						OutputDebugString("Primary buffer format was set.\n");
					}
					else
					{

					}
				}
			}
			else
			{

			}
			// Create a secondary buffer which we will actually be writing to
			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			HRESULT Error = directSound->CreateSoundBuffer(&bufferDescription, &GlobalSoundBuffer, 0);
			if (SUCCEEDED(Error)) {
				OutputDebugString("Secondary buffer was created.\n");
			}
			else {
			}
		}
	}
}

internal Win32WindowDimension Win32GetWindowDimension(HWND window)
{
	Win32WindowDimension result;
	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.Width = clientRect.right - clientRect.left;
	result.Height = clientRect.bottom - clientRect.top;
	return result;
}

internal void DrawWeirdGradient(Win32OffscreenBuffer* buffer, int xOffset, int yOffset)
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

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int width, int height)
{
	if (buffer->Memory)
	{
		VirtualFree(buffer->Memory, 0, MEM_RELEASE);
	}

	buffer->Width = width;
	buffer->Height = height;

	buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
	buffer->Info.bmiHeader.biWidth = buffer->Width;
	//Set the biHeight to negative BitmapHeight so the bitmap is considered a top-down DIB and its origin is the upper-left corner.
	//See https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376(v=vs.85).aspx
	buffer->Info.bmiHeader.biHeight = -buffer->Height;
	buffer->Info.bmiHeader.biPlanes = 1;
	buffer->Info.bmiHeader.biBitCount = 32; //RGB is just 24, but will ask for 32 for D-Word Alignment (cpu accesses memory at multiple of 4 easier :) )
	buffer->Info.bmiHeader.biCompression = BI_RGB;

	int bytesPerPixel = 4;
	int	bitmapMemorySize = bytesPerPixel * buffer->Width * buffer->Height;
	buffer->Memory = VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	buffer->Pitch = buffer->Width * bytesPerPixel;
	DrawWeirdGradient(buffer, 0, 0);
}

internal void Win32DisplayBufferInWindow(HDC deviceContext, Win32OffscreenBuffer* buffer, int windowWidth, int windowHeight)
{
	StretchDIBits(deviceContext,
		0, 0, windowWidth, windowHeight //destination
		, 0, 0, buffer->Width, buffer->Height, //source
		buffer->Memory,	&(buffer->Info), DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message
						, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{
	case WM_SIZE:
	{
	}break;
	case WM_DESTROY:
	{
		OutputDebugString("WM_DESTORY\n");
		GlobalRunning = false;
	}break;
	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_KEYDOWN:
	{
		WPARAM vkCode = wParam;
		bool wasDown = ((lParam & (1 << 30)) != 0);
		bool isDown = ((lParam & (1 << 31)) == 0);
		if (wasDown != isDown)
		{
			if (vkCode == VK_F4)
			{
				bool altDown = ((1 << 29) & lParam) != 0;
				if (altDown)
					GlobalRunning = false;
			}
			else if (vkCode == 'A')
			{

			}
			else if (vkCode == 'S')
			{

			}
			else if (vkCode == 'D')
			{

			}
			else if (vkCode == 'Q')
			{

			}
			else if (vkCode == 'W')
			{

			}
			else if (vkCode == 'E')
			{

			}
			else if (vkCode == VK_UP)
			{
				if (!wasDown)
					GlobalSoundOutput.UpdateToneHz(GlobalSoundOutput.ToneHz + 128);
			}
			else if (vkCode == VK_DOWN)
			{
				if (!wasDown)
					GlobalSoundOutput.UpdateToneHz(GlobalSoundOutput.ToneHz - 128);
			}
			else if (vkCode == VK_LEFT)
			{

			}
			else if (vkCode == VK_RIGHT)
			{

			}
			else if (vkCode == VK_ESCAPE)
			{
				OutputDebugString("ESC: ");
				if (isDown)
				{
					OutputDebugString("IsDown");
				}
				if (wasDown)
				{
					OutputDebugString("WasDown");
				}
				OutputDebugString("\n");
			}
			else if (vkCode == VK_SPACE)
			{

			}
		}
	}break;
	case WM_CLOSE:
	{
		OutputDebugString("WM_CLOSE\n");
		GlobalRunning = false;
	}break;
	case WM_ACTIVATEAPP:
	{
		OutputDebugString("WM_ACTIVATEAPP\n");
	}break;
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		Win32WindowDimension windowDimension = Win32GetWindowDimension(window);
		Win32DisplayBufferInWindow(deviceContext, &GlobalBackbuffer, windowDimension.Width, windowDimension.Height);
		EndPaint(window, &paint);
	}break;
	default:
	{
		result = DefWindowProc(window, message, wParam, lParam);
	}break;
	}
	return result;
}

internal void Win32FillSoundBuffer(Win32SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite)
{
	VOID* region1;
	DWORD region1Size;
	VOID* region2;
	DWORD region2Size;

	if (SUCCEEDED(GlobalSoundBuffer->Lock(byteToLock, bytesToWrite,
											&region1, &region1Size,
											&region2, &region2Size, 0)))
	{
		DWORD region1SampleCount = region1Size / soundOutput->BytesPerSample;
		int16* sampleOut = (int16*)region1;
		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
		{
			real32 sineValue = sinf(GlobalSoundOutput.TSine);
			int16 sampleValue = (int16)(sineValue * soundOutput->ToneVolume);
			*sampleOut++ = sampleValue;
			*sampleOut++ = sampleValue;
			GlobalSoundOutput.TSine += (2.0f * PI32) / (real32)soundOutput->WavePeriod;
			soundOutput->RunningSampleIndex++;
		}

		DWORD region2SampleCount = region2Size / soundOutput->BytesPerSample;
		sampleOut = (int16*)region2;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
		{
			real32 sineValue = sinf(GlobalSoundOutput.TSine);
			int16 sampleValue = (int16)(sineValue * soundOutput->ToneVolume);
			*sampleOut++ = sampleValue;
			*sampleOut++ = sampleValue;
			GlobalSoundOutput.TSine += (2.0f * PI32) / (real32)soundOutput->WavePeriod;
			soundOutput->RunningSampleIndex++;
		}
		GlobalSoundBuffer->Unlock(region1, region1SampleCount, region2, region2SampleCount);
	}
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow
)
{
	Win32LoadXInput();
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WNDCLASS windowClass = {}; //init all struct members to 0
	// TODO: 
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = MainWindowCallback;//callback from windows when our window needs to do somwthing
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&windowClass))
	{
		HWND window = CreateWindowEx(0, windowClass.lpszClassName, "Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
		if (window)
		{
			GlobalSoundOutput.SamplesPerSecond = 48000;
			GlobalSoundOutput.BytesPerSample = sizeof(int16) * 2;
			GlobalSoundOutput.RunningSampleIndex = 0;
			GlobalSoundOutput.ToneHz = 256;
			GlobalSoundOutput.WavePeriod = GlobalSoundOutput.SamplesPerSecond / GlobalSoundOutput.ToneHz;
			GlobalSoundOutput.SoundBufferSize = GlobalSoundOutput.SamplesPerSecond * GlobalSoundOutput.BytesPerSample;
			GlobalSoundOutput.LatencySampleCount = GlobalSoundOutput.SamplesPerSecond / 15;
			GlobalSoundOutput.ToneVolume = 512;
			Win32InitDSound(window, GlobalSoundOutput.SamplesPerSecond, GlobalSoundOutput.SoundBufferSize);
			Win32FillSoundBuffer(&GlobalSoundOutput, 0, GlobalSoundOutput.LatencySampleCount * GlobalSoundOutput.BytesPerSample);
			GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

			HDC deviceContext = GetDC(window);
			int xOffset = 0;

			GlobalRunning = true;
			while (GlobalRunning)
			{
				MSG message; //define it inside the loop, it will not make any difference to the compiler 
							 // + u get the benefit of not beign able to mistakenly reference it outside the loop.
							 //Unless of course the type is a class that has a destructor that u dont want to be called at
							 // every iteration.
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
						GlobalRunning = false;
					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				//polling for controller input
				for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
				{
					XINPUT_STATE controllerState;
					if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
					{
						//ERROR_SUCCESS: controller is plugged in
						XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
						bool32 up = XINPUT_GAMEPAD_DPAD_UP & pad->wButtons;
						bool32 down = XINPUT_GAMEPAD_DPAD_DOWN & pad->wButtons;
						bool32 left = XINPUT_GAMEPAD_DPAD_LEFT & pad->wButtons;
						bool32 right = XINPUT_GAMEPAD_DPAD_RIGHT & pad->wButtons;
						bool32 start = XINPUT_GAMEPAD_START & pad->wButtons;
						bool32 back = XINPUT_GAMEPAD_BACK & pad->wButtons;
						bool32 rightShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER & pad->wButtons;
						bool32 leftTShoulder = XINPUT_GAMEPAD_LEFT_SHOULDER & pad->wButtons;
						bool32 aButton = XINPUT_GAMEPAD_A & pad->wButtons;
						bool32 bButton = XINPUT_GAMEPAD_B & pad->wButtons;
						bool32 xButton = XINPUT_GAMEPAD_X & pad->wButtons;
						bool32 yButton = XINPUT_GAMEPAD_Y & pad->wButtons;

						int16 stickX = pad->sThumbLX;
						int16 stickY = pad->sThumbLY;
					}
					else
					{

					}
				}
				DrawWeirdGradient(&GlobalBackbuffer, xOffset, 0);
				Win32WindowDimension windowDimension = Win32GetWindowDimension(window);
				Win32DisplayBufferInWindow(deviceContext, &GlobalBackbuffer, windowDimension.Width, windowDimension.Height);
				xOffset++;

				DWORD playCursor;
				DWORD writeCursor;
				if (SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
				{
					DWORD byteToLock = (GlobalSoundOutput.RunningSampleIndex * GlobalSoundOutput.BytesPerSample) % GlobalSoundOutput.SoundBufferSize;
					DWORD bytesToWrite;
					DWORD targetCursor = (playCursor + GlobalSoundOutput.LatencySampleCount * GlobalSoundOutput.BytesPerSample) 
										 % GlobalSoundOutput.SoundBufferSize;
					
					if (byteToLock > targetCursor)
					{
						bytesToWrite = GlobalSoundOutput.SoundBufferSize - byteToLock;
						bytesToWrite += targetCursor;
					}
					else
					{
						bytesToWrite = targetCursor - byteToLock;
					}

					Win32FillSoundBuffer(&GlobalSoundOutput, byteToLock, bytesToWrite);
				}
			}
			//We specified CS_OWNDC => we are telling windows that we want our own DC that we wont have to return to DC pool
			// => there is no need to Release it.
		}
		else
		{

		}
	}
	else
	{
		
	}
	return 0;
}