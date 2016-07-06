#include <windows.h>
#include   <string>   
using   namespace   std;

//----------��������---------------
void init(HWND hWnd, HINSTANCE hInstance);
void pix(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps);//����,Ҳ�����������
void rect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps);//----������----
void fillrect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//������
void linerect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//�߿����
void triangle(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, POINT Pt[3], PAINTSTRUCT &Ps);//������
void ellipse(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//��Բ
void DrawContent(HWND hWnd);

//-----�ص�����-----------------
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
	CreateWindow(TEXT("BUTTON"), TEXT("����"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 0, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("����"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 90, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("������"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 180, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("�߿����"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 270, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("������"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 360, 150, 80, hWnd, NULL, hInstance, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("��Բ"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 450, 150, 80, hWnd, NULL, hInstance, NULL);
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
		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("����"));
		if (DWORD(lParam) == int(h))
		{
			pix(hWnd, hDC, NewBrush, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("����"));
		if (DWORD(lParam) == int(h))
		{
			rect(hWnd, hDC, NewBrush, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("������"));
		if (DWORD(lParam) == int(h))
		{
			fillrect(hWnd, hDC, NewBrush, r, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("�߿����"));
		if (DWORD(lParam) == int(h))
		{
			linerect(hWnd, hDC, NewBrush, r, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("������"));
		if (DWORD(lParam) == int(h))
		{
			triangle(hWnd, hDC, NewBrush, Pt, Ps);
			break;
		}

		h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("��Բ"));
		if (DWORD(lParam) == int(h))
		{
			ellipse(hWnd, hDC, NewBrush, r, Ps);
			break;
		}
	case WM_PAINT:
		DrawContent(hWnd);//Ϊʲô������һ���ʼ��ť�ͻ���ʾ�������ӵĻ���ť��Ҫ�ػ��Ժ���ʾ
		break;

	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}



//����
void pix(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps)
{
	//hDC = BeginPaint(hWnd, &Ps);//��BeginPaint�Ļ���Ϣһ��ҪWM_PAINT��Ӧ������WM_PAINT����ֻ��DrawContent(hWnd);
	//������getDC
	hDC = GetDC(hWnd);
	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
	//SelectObject(hDC, NewBrush);
	for (int x = 500; x<600; x++)//����һ����̫С�ˣ��������ﻨ��100���㹹���߶�
		SetPixel(hDC, x, 50, RGB(255, 0, 0));
	//DeleteObject(NewBrush);
	//EndPaint(hWnd, &Ps);
	ReleaseDC(hWnd, hDC);
}
//����
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

//������
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

//�߿����
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
//������
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

//��Բ
void ellipse(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps)//��Բ
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
	hDC = BeginPaint(hWnd, &Ps);//��BeginPaint�Ļ���Ϣһ��ҪWM_Paint��Ӧ
	EndPaint(hWnd, &Ps);

}
