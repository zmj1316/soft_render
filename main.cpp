#include <windows.h>
#include   <string>   
using   namespace   std;

//----------函数声明---------------
void init(HWND hWnd, HINSTANCE hInstance);
void pix(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps);//画点,也就是像素输出
void rect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps);//----画矩形----
void fillrect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//填充矩形
void linerect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//线框矩形
void triangle(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, POINT Pt[3], PAINTSTRUCT &Ps);//三角形
void ellipse(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//椭圆
void DrawContent(HWND hWnd);

//-----回调函数-----------------
LRESULT CALLBACK WindProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	WNDCLASSEX  WndCls;

	MSG         Msg;
	WndCls.cbSize = sizeof(WndCls);
	WndCls.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	WndCls.lpfnWndProc = WindProcedure;
	WndCls.cbClsExtra = 0;
	WndCls.cbWndExtra = 0;
	WndCls.hInstance = hInstance;
	WndCls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndCls.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndCls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndCls.lpszMenuName = NULL;
	WndCls.lpszClassName = TEXT("MAIN");
	WndCls.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	RegisterClassEx(&WndCls);

	HWND  hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
		TEXT("MAIN"), TEXT("TEST"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, 820, 820,
		NULL, NULL, hInstance, NULL);
	init(hWnd, hInstance);
	UpdateWindow(hWnd);
	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return static_cast<int>(Msg.wParam);
}

void init(HWND hWnd, HINSTANCE hInstance)
{
	CreateWindow(TEXT("BUTTON"), TEXT("画点"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 0, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("矩形"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 90, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("填充矩形"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 180, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("线框矩形"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 270, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("三角形"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 360, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("椭圆"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 450, 150, 80, hWnd, NULL, hInstance, NULL);
}

LRESULT CALLBACK WindProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{

	HDC         hDC;
	PAINTSTRUCT Ps;
	HBRUSH      NewBrush;
	RECT	r;
	POINT Pt[3];

	switch (Msg)
	{
	case WM_COMMAND:
		HWND h;
		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("画点"));
		if (DWORD(lParam) == int(h))
		{
			pix(hWnd, hDC, NewBrush, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("矩形"));
		if (DWORD(lParam) == int(h))
		{
			rect(hWnd, hDC, NewBrush, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("填充矩形"));
		if (DWORD(lParam) == int(h))
		{
			fillrect(hWnd, hDC, NewBrush, r, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("线框矩形"));
		if (DWORD(lParam) == int(h))
		{
			linerect(hWnd, hDC, NewBrush, r, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("三角形"));
		if (DWORD(lParam) == int(h))
		{
			triangle(hWnd, hDC, NewBrush, Pt, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("椭圆"));
		if (DWORD(lParam) == int(h))
		{
			ellipse(hWnd, hDC, NewBrush, r, Ps);
			break;
		}
	case WM_PAINT:
		DrawContent(hWnd);//为什么加了这一句后开始按钮就会显示，而不加的话按钮需要重绘以后显示
		break;

	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}



//画点
void pix(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps)
{
	//hDC = BeginPaint(hWnd, &Ps);//用BeginPaint的话消息一定要WM_PAINT响应，但是WM_PAINT里面只有DrawContent(hWnd);
	//所以用getDC
	hDC = GetDC(hWnd);
	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
	//SelectObject(hDC, NewBrush);
	for (int x = 500; x<600; x++)//由于一个点太小了，所以这里花了100个点构成线段
		SetPixel(hDC, x, 50, RGB(255, 0, 0));
	//DeleteObject(NewBrush);
	//EndPaint(hWnd, &Ps);
	ReleaseDC(hWnd, hDC);
}
//矩形
void rect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps)
{
	//hDC = BeginPaint(hWnd, &Ps);
	hDC = GetDC(hWnd);
	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
	SelectObject(hDC, NewBrush);
	Rectangle(hDC, 400, 400, 500, 500);
	DeleteObject(NewBrush);
	ReleaseDC(hWnd, hDC);
	//EndPaint(hWnd, &Ps);
}

//填充矩形
void fillrect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps)
{
	//hDC = BeginPaint(hWnd, &Ps);
	hDC = GetDC(hWnd);
	NewBrush = CreateSolidBrush(RGB(25, 25, 5));
	SelectObject(hDC, NewBrush);
	SetRect(&r, 200, 200, 250, 250);
	FillRect(hDC, &r, NewBrush);
	DeleteObject(NewBrush);
	//EndPaint(hWnd, &Ps);
	ReleaseDC(hWnd, hDC);
}

//线框矩形
void linerect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps)
{
	//hDC = BeginPaint(hWnd, &Ps);
	hDC = GetDC(hWnd);
	NewBrush = CreateSolidBrush(RGB(25, 25, 5));
	SelectObject(hDC, NewBrush);
	SetRect(&r, 250, 250, 400, 400);
	FrameRect(hDC, &r, NewBrush);
	DeleteObject(NewBrush);
	DeleteObject(NewBrush);
	//EndPaint(hWnd, &Ps);
	ReleaseDC(hWnd, hDC);
}
//三角形
void triangle(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, POINT Pt[3], PAINTSTRUCT &Ps)
{
	//hDC = BeginPaint(hWnd, &Ps);
	hDC = GetDC(hWnd);
	NewBrush = CreateSolidBrush(RGB(50, 50, 50));
	SelectObject(hDC, NewBrush);
	Pt[0].x = 425; Pt[0].y = 40;
	Pt[1].x = 395; Pt[1].y = 70;
	Pt[2].x = 455; Pt[2].y = 70;
	Polygon(hDC, Pt, 3);
	DeleteObject(NewBrush);

	NewBrush = CreateSolidBrush(RGB(0, 255, 0));
	SelectObject(hDC, NewBrush);
	Pt[0].x = 365; Pt[0].y = 110;
	Pt[1].x = 395; Pt[1].y = 80;
	Pt[2].x = 395; Pt[2].y = 140;
	Polygon(hDC, Pt, 3);
	DeleteObject(NewBrush);

	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
	SelectObject(hDC, NewBrush);
	Pt[0].x = 485; Pt[0].y = 110;
	Pt[1].x = 455; Pt[1].y = 80;
	Pt[2].x = 455; Pt[2].y = 140;
	Polygon(hDC, Pt, 3);
	DeleteObject(NewBrush);

	NewBrush = CreateSolidBrush(RGB(0, 0, 255));
	SelectObject(hDC, NewBrush);
	Pt[0].x = 425; Pt[0].y = 180;
	Pt[1].x = 455; Pt[1].y = 150;
	Pt[2].x = 395; Pt[2].y = 150;
	Polygon(hDC, Pt, 3);
	DeleteObject(NewBrush);
	//EndPaint(hWnd, &Ps);
	ReleaseDC(hWnd, hDC);
}

//椭圆
void ellipse(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps)//椭圆
{
	//hDC = BeginPaint(hWnd, &Ps);
	hDC = GetDC(hWnd);
	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
	SelectObject(hDC, NewBrush);
	Ellipse(hDC, 500, 500, 600, 600);
	DeleteObject(NewBrush);
	//EndPaint(hWnd, &Ps);
	ReleaseDC(hWnd, hDC);
}

void DrawContent(HWND hWnd)
{
	HDC         hDC;
	PAINTSTRUCT Ps;
	hDC = BeginPaint(hWnd, &Ps);//用BeginPaint的话消息一定要WM_Paint响应
	EndPaint(hWnd, &Ps);

}
