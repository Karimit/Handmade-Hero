#include "windows.h"

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message
						, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{
	case WM_SIZE:
	{
		OutputDebugString("WM_SIZE\n");
	}break;
	case WM_DESTROY:
	{
		OutputDebugString("WM_DESTORY\n");
	}break;
	case WM_CLOSE:
	{
		OutputDebugString("WM_CLOSE\n");
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
		static DWORD op = WHITENESS; //dont try this at home

		if (op == WHITENESS)
		{
			op = BLACKNESS;
		}
		else
		{
			op = WHITENESS;
		}
		PatBlt(deviceContext, x, y, width, height, op);
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
			for (;;)
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