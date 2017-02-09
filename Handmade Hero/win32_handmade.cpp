// TODO: THIS IS NOT A FINAL PLATFORM LAYER!!
/**
 *   - Saved game locations
 *   - Getting a handle to our own executable file
 *   - Asset loading path
 *   - Threading (launch a thread)
 *   - Raw Input (support for multiple keyboards)
 *   - Sleep/timeBeginPeriod
 *   - ClipCursor() (multimonitor support)
 *   - Fullscreen support
 *   - WM_SETCURSOR (control cursor visibility)
 *   - QueryCanelAutoplay
 *   - WM_ACTIVATEAPP (for when we are not active application)
 *   - Blit speed improvemetns (BitBlt)
 *   - Hardware acceleration (OpenGL or Direct3D or BOTH??)
 *   - GetKeyboardLayout (for French keybords, international WASD support)
 *
 *   Just a partial list of stuff!!
 */
#include "windows.h"
#include "xinput.h"
#include "dsound.h"
#include "stdio.h"
#include "util.h"

#include "win32_handmade.h"
#include "handmade.h"

global_variable bool GlobalRunning;
global_variable Win32OffscreenBuffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer; // The actual sound buffer that we will write to.
global_variable Win32SoundOutput GlobalSoundOutput;

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
	buffer->Memory = VirtualAlloc(NULL, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	buffer->Pitch = buffer->Width * bytesPerPixel;
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
				{
				}
			}
			else if (vkCode == VK_DOWN)
			{
				if (!wasDown)
				{

				}
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

internal void Win32ClearSoundBuffer(Win32SoundOutput* soundOutput)
{
	VOID* region1;
	DWORD region1Size;
	VOID* region2;
	DWORD region2Size;
	if (SUCCEEDED(GlobalSoundBuffer->Lock(0, soundOutput->SoundBufferSize,
		&region1, &region1Size,
		&region2, &region2Size, 0)))
	{
		uint8* destSample = (uint8*)region1;
		for (DWORD byteIndex = 0; byteIndex < region1Size; byteIndex++)
		{
			*destSample++ = 0;
		}

		destSample = (uint8*)region2;
		for (DWORD byteIndex = 0; byteIndex < region2Size; byteIndex++)
		{
			*destSample++ = 0;
		}
		GlobalSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

internal void Win32FillSoundBuffer(Win32SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite,
	GameSoundOutputBuffer* sourceBuffer)
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
		int16* destSample = (int16*)region1;
		int16* sourceSample = sourceBuffer->Samples;
		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
		{
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			soundOutput->RunningSampleIndex++;
		}

		DWORD region2SampleCount = region2Size / soundOutput->BytesPerSample;
		destSample = (int16*)region2;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
		{
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			soundOutput->RunningSampleIndex++;
		}
		GlobalSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

internal void Win32ProcessXInputDigitalButtons(WORD xInputButtonState, GameButtonState* newState, GameButtonState* oldState, DWORD buttonBit)
{
	newState->EnddedDown = (buttonBit & xInputButtonState) == buttonBit;
	newState->HalfTransitionCount = (oldState->EnddedDown != newState->EnddedDown) ? 1 : 0;
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow
)
{
	LARGE_INTEGER perfFrequencyResult;
	QueryPerformanceFrequency(&perfFrequencyResult);
	int64 perfCountFrequency = perfFrequencyResult.QuadPart;

	Win32LoadXInput();
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WNDCLASS windowClass = {}; //init all struct members to 0
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
			GlobalSoundOutput.SoundBufferSize = GlobalSoundOutput.SamplesPerSecond * GlobalSoundOutput.BytesPerSample;
			GlobalSoundOutput.LatencySampleCount = GlobalSoundOutput.SamplesPerSecond / 15;
			Win32InitDSound(window, GlobalSoundOutput.SamplesPerSecond, GlobalSoundOutput.SoundBufferSize);
			Win32ClearSoundBuffer(&GlobalSoundOutput);
			GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

			int16* soundSamples = (int16*)VirtualAlloc(NULL, GlobalSoundOutput.SoundBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			HDC deviceContext = GetDC(window);

			GlobalRunning = true;

#if HANDMADE_INTERNAL
			LPVOID baseAddress = (LPVOID)Terabytes(2);
#else
			LPVOID baseAddress = 0;
#endif
			GameMemory gameMemory = {};
			gameMemory.PermenantStorageSize = Megabytes(64);
			gameMemory.TransientStorageSize = Gigabytes(1);
			uint64 totalSize = gameMemory.PermenantStorageSize + gameMemory.TransientStorageSize;
			gameMemory.PermenantStorage = VirtualAlloc(baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			gameMemory.TransientStorage = (uint8*)gameMemory.PermenantStorage + gameMemory.PermenantStorageSize;

			if (gameMemory.PermenantStorage && gameMemory.TransientStorage && soundSamples)
			{
				GameInput input[2] = {};
				GameInput* oldInput = &input[0];
				GameInput* newInput = &input[1];

				LARGE_INTEGER lastCounter;
				QueryPerformanceCounter(&lastCounter);

				uint64 lastCycleCount = __rdtsc();
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
					int maxControllerCount = XUSER_MAX_COUNT;
					if (maxControllerCount > ArrayCount(newInput->Controllers))
					{
						maxControllerCount = ArrayCount(newInput->Controllers);
					}
					//polling for controller input
					for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
					{
						GameControllerInput* oldController = &oldInput->Controllers[controllerIndex];
						GameControllerInput* newController = &newInput->Controllers[controllerIndex];

						XINPUT_STATE controllerState;
						if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
						{
							//ERROR_SUCCESS: controller is plugged in
							newController->IsAnalog = true;
							XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
							bool32 up = XINPUT_GAMEPAD_DPAD_UP & pad->wButtons;
							bool32 down = XINPUT_GAMEPAD_DPAD_DOWN & pad->wButtons;
							bool32 left = XINPUT_GAMEPAD_DPAD_LEFT & pad->wButtons;
							bool32 right = XINPUT_GAMEPAD_DPAD_RIGHT & pad->wButtons;

							newController->StartX = oldController->EndX;
							newController->StartY = oldController->EndY;

							real32 x;
							if (pad->sThumbLX < 0)
							{
								x = (real32)pad->sThumbLX / 32768;
							}
							else
							{
								x = (real32)pad->sThumbLX / 32767;
							}
							real32 y;
							if (pad->sThumbLY < 0)
							{
								y = (real32)pad->sThumbLY / 32768;
							}
							else
							{
								y = (real32)pad->sThumbLY / 32767;
							}

							newController->MinX = newController->MaxX = newController->EndX = x;

							Win32ProcessXInputDigitalButtons(pad->wButtons, &oldController->Down
								, &newController->Down, XINPUT_GAMEPAD_A);
							Win32ProcessXInputDigitalButtons(pad->wButtons, &oldController->Right
								, &newController->Right, XINPUT_GAMEPAD_B);
							Win32ProcessXInputDigitalButtons(pad->wButtons, &oldController->Left
								, &newController->Left, XINPUT_GAMEPAD_X);
							Win32ProcessXInputDigitalButtons(pad->wButtons, &oldController->Up
								, &newController->Up, XINPUT_GAMEPAD_Y);
							Win32ProcessXInputDigitalButtons(pad->wButtons, &oldController->RightShoulder
								, &newController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);
							Win32ProcessXInputDigitalButtons(pad->wButtons, &oldController->LeftShoulder
								, &newController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);

							/*bool32 start = XINPUT_GAMEPAD_START & pad->wButtons;
							bool32 back = XINPUT_GAMEPAD_BACK & pad->wButtons;*/
						}
						else
						{

						}
					}

					DWORD playCursor;
					DWORD writeCursor;
					DWORD byteToLock;
					DWORD bytesToWrite;
					DWORD targetCursor;
					bool32 soundIsValid = false;

					if (SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
					{
						byteToLock = (GlobalSoundOutput.RunningSampleIndex * GlobalSoundOutput.BytesPerSample) % GlobalSoundOutput.SoundBufferSize;
						targetCursor = (playCursor + GlobalSoundOutput.LatencySampleCount * GlobalSoundOutput.BytesPerSample)
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
						soundIsValid = true;
					}

					GameSoundOutputBuffer soundBuffer = {};
					soundBuffer.SamplesPerSecond = GlobalSoundOutput.SamplesPerSecond;
					soundBuffer.SampleCount = bytesToWrite / GlobalSoundOutput.BytesPerSample; // We are targeting 30 frames per second for now.
					soundBuffer.Samples = soundSamples;

					GameOffscreenBuffer buffer = {};
					buffer.Memory = GlobalBackbuffer.Memory;
					buffer.Width = GlobalBackbuffer.Width;
					buffer.Height = GlobalBackbuffer.Height;
					buffer.Pitch = GlobalBackbuffer.Pitch;
					GameUpdateAndRender(&gameMemory, &buffer, &soundBuffer, newInput);


					if (soundIsValid)
					{
						Win32FillSoundBuffer(&GlobalSoundOutput, byteToLock, bytesToWrite, &soundBuffer);
					}

					Win32WindowDimension windowDimension = Win32GetWindowDimension(window);
					Win32DisplayBufferInWindow(deviceContext, &GlobalBackbuffer, windowDimension.Width, windowDimension.Height);

					uint64 endCycleCount = __rdtsc();
					int64 cyclesElapsed = endCycleCount - lastCycleCount;

					LARGE_INTEGER endCounter;
					QueryPerformanceCounter(&endCounter);

					int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
					real32 msPerFrame = ((1000.f * (real32)counterElapsed) / (real32)perfCountFrequency);
					real32 fps = (real32)perfCountFrequency / counterElapsed;
					real32 megaCyclesPerFrame = (real32)(cyclesElapsed / (1000 * 1000));

#if 0
					char buffer[256];
					sprintf_s(buffer, "%fmsf, %ffps, %fmcpf\n",
						msPerFrame, fps, megaCyclesPerFrame);
					OutputDebugString(buffer);
#endif // 0


					lastCounter = endCounter;
					lastCycleCount = endCycleCount;
				}

				//We specified CS_OWNDC => we are telling windows that we want our own DC that we wont have to return to DC pool
				// => there is no need to Release it.

				GameInput* temp = newInput;
				newInput = oldInput;
				oldInput = temp;
			}
			else
			{

			}
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