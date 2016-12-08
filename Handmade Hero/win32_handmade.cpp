#include "windows.h"
#include "stdint.h"

typedef uint8_t uint8;
typedef uint8_t uint16;
typedef uint32_t uint32;
typedef uint32_t uint64;

#define internal static
#define local_persist static
#define global_variable static

global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;

internal void Win32ResizeDIBSection(int width, int height)
{
	if (BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = width;
	BitmapHeight = height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	//Set the biHeight to negative BitmapHeight so the bitmap is considered a top-down DIB and its origin is the upper-left corner.
	//See https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376(v=vs.85).aspx
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32; //RGB is just 24, but will ask for 32 for D-Word Alignment (cpu accesses memory at multiple of 4 easier :) )
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	int bytesPerPixel = 4;
	int	bitmapMemorySize = 4 * width * height;
	BitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC deviceContext, RECT* windowRect, int x, int y, int width, int height)
{
	//passed a pointer to window rect because otherwise the whole window rect would have been unnecessarly passed on the stack.
	int windowWidth = windowRect->right - windowRect->left;
	int windowHeight = windowRect->bottom - windowRect->top;
	StretchDIBits(deviceContext,
		windowRect->left, windowRect->top, windowWidth, windowHeight //destination
		, 0, 0, BitmapWidth, BitmapHeight, //source
		BitmapMemory,
		&BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message
						, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{
	case WM_SIZE:
	{
		RECT clientRect;
		GetClientRect(window, &clientRect);
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;
		Win32ResizeDIBSection(width, height);
		OutputDebugString("WM_SIZE\n");
	}break;
	case WM_DESTROY:
	{
		OutputDebugString("WM_DESTORY\n");
		Running = false;
	}break;
	case WM_CLOSE:
	{
		OutputDebugString("WM_CLOSE\n");
		Running = false;
	}break;
	case WM_ACTIVATEAPP:
	{
		OutputDebugString("WM_ACTIVATEAPP\n");
	}break;
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		int x = paint.rcPaint.left;
		int y = paint.rcPaint.top;
		int width = paint.rcPaint.right - paint.rcPaint.left;
		int height = paint.rcPaint.bottom - paint.rcPaint.top;

		RECT windowRect;
		GetClientRect(window, &windowRect);
		Win32UpdateWindow(deviceContext, &windowRect, x, y, width, height);
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
	WNDCLASS windowClass = {}; //init all struct members to 0
	//windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = MainWindowCallback;//callback from windows when our window needs to do somwthing
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&windowClass))
	{
		HWND windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
		if (windowHandle)
		{
			MSG message;
			Running = true;
			while (Running)
			{
				BOOL messageResult = GetMessage(&message, 0 /*get messages from all windows belonging to us*/,
					0, 0);
				if (messageResult > 0)
				{
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				else
				{
					break;
				}
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