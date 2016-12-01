#include "windows.h"

#define internal static
#define local_persist static
#define global_variable static

global_variable bool Running;

//TODO: make them global vars
BITMAPINFO BitmapInfo;
void* BitmapMemory;
HBITMAP BitmapHandle;
HDC BitmapDeviceContext;

//TODO: make it internal
void Win32ResizeDIBSection(int width, int height)
{
	if (BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}
	if (!BitmapDeviceContext)
	{
		BitmapDeviceContext = CreateCompatibleDC(0);
	}
	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = width;
	BitmapInfo.bmiHeader.biHeight = height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS,
		&BitmapMemory, 0, 0);
}

//TODO: make it internal
void Win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height)
{
	StretchDIBits(deviceContext, x, y, width, height, x, y, width, height, BitmapMemory,
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
		Win32UpdateWindow(deviceContext, x, y, width, height);
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