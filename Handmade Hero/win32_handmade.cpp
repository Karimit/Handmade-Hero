#include "windows.h"
#include "stdint.h"
#include "xinput.h"

typedef uint8_t uint8;
typedef uint8_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef int32 bool32;
typedef uint32_t uint64;

#define internal static
#define local_persist static
#define global_variable static

struct Win32OffscreenBuffer
{
	int Width;
	int Height;
	BITMAPINFO Info;
	void* Memory;
	int Pitch;
};

global_variable bool GlobalRunning;
global_variable Win32OffscreenBuffer GlobalBackbuffer;

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

internal Win32WindowDimension Win32GetWindowDimension(HWND window)
{
	Win32WindowDimension result;
	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.Width = clientRect.right - clientRect.left;
	result.Height = clientRect.bottom - clientRect.top;
	return result;
}

internal void DrawWieredGradient(Win32OffscreenBuffer buffer, int xOffset, int yOffset)
{
	uint32* pixel = (uint32*)buffer.Memory;
	uint8* row = (uint8*)buffer.Memory;
	for (int y = 0; y < buffer.Height; y++)
	{
		pixel = (uint32*)row;
		for (int x = 0; x < buffer.Width; x++)
		{
			uint8 blue = x + xOffset;
			uint8 green = y + yOffset;
			*pixel++ = (uint32)((green << 8) | blue);
		}
		row += buffer.Pitch;
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
	DrawWieredGradient(*buffer, 0, 0);
}

internal void Win32DisplayBufferInWindow(HDC deviceContext, Win32OffscreenBuffer buffer, int windowWidth, int windowHeight)
{
	//Passed the whole rect and not a pointer to it. Maybe more efficent as this function is small and might be inlined anyway
	// and the compiler will use the same Rect that is already on the stack.
	StretchDIBits(deviceContext,
		0, 0, windowWidth, windowHeight //destination
		, 0, 0, buffer.Width, buffer.Height, //source
		buffer.Memory,	&(buffer.Info), DIB_RGB_COLORS, SRCCOPY);
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

			}
			else if (vkCode == VK_DOWN)
			{

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
		Win32DisplayBufferInWindow(deviceContext, GlobalBackbuffer, windowDimension.Width, windowDimension.Height);
		EndPaint(window, &paint);
	}break;
	default:
	{
		result = DefWindowProc(window, message, wParam, lParam);
	}break;
	}
	return result;
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
			GlobalRunning = true;
			HDC deviceContext = GetDC(window);
			int xOffset = 0;
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

				DrawWieredGradient(GlobalBackbuffer, xOffset, 0);
				Win32WindowDimension windowDimension = Win32GetWindowDimension(window);
				Win32DisplayBufferInWindow(deviceContext, GlobalBackbuffer, windowDimension.Width, windowDimension.Height);
				xOffset++;
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