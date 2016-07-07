#include <windows.h>
#include   <string>   
using   namespace   std;

#include "Cube.h"
#include "matrix.h"
#include <math.h>
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
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 820,
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

void WT(Reactangular &r0)
{
    // 世界变换
    for (size_t i = 0; i < 4; i++)
    {
        float* vec = r0.vertexs[i].vec;
        float new_vec[4];
        for (size_t i = 0; i < 4; i++)
        {
            new_vec[i] = r0.transform.right.vec[i] * vec[0]
                + r0.transform.up.vec[i] * vec[1]
                + r0.transform.forward.vec[i] * vec[2]
                + r0.transform.position.vec[i] * vec[3];
        }
        memcpy(vec, new_vec, 4 * sizeof(float));
    }
}

void get_Pt(POINT* Pt, Reactangular& r0)
{
    for (size_t i = 0; i < 4; i++)
    {
        Pt[i].x = 10 * r0.vertexs[i].vec[0] * 50 / r0.vertexs[i].vec[2];
        Pt[i].y = 10 * r0.vertexs[i].vec[1] * 50 / r0.vertexs[i].vec[2];
        //Pt[i].x = r0.vertexs[i].vec[0] * (50 / r0.vertexs[i].vec[2]);
        //Pt[i].y = r0.vertexs[i].vec[1] * (50 / r0.vertexs[i].vec[2]);
    }
}

void init(HWND hWnd, HINSTANCE hInstance)
{
//	CreateWindow(TEXT("BUTTON"), TEXT("画点"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 0, 150, 80, hWnd, NULL, hInstance, NULL);
//	CreateWindow(TEXT("BUTTON"), TEXT("矩形"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 90, 150, 80, hWnd, NULL, hInstance, NULL);
//	CreateWindow(TEXT("BUTTON"), TEXT("填充矩形"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 180, 150, 80, hWnd, NULL, hInstance, NULL);
//	CreateWindow(TEXT("BUTTON"), TEXT("线框矩形"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 270, 150, 80, hWnd, NULL, hInstance, NULL);
//	CreateWindow(TEXT("BUTTON"), TEXT("三角形"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 360, 150, 80, hWnd, NULL, hInstance, NULL);
//	CreateWindow(TEXT("BUTTON"), TEXT("椭圆"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 450, 150, 80, hWnd, NULL, hInstance, NULL);
    
//    Cube cube;
//    float Tx = -30;
//    float Ty = -30;
//    float Tz = 50;
    HDC hDC = GetDC(hWnd);


    //Reactangular r0;
    //// 第一个面
    //r0.transform.position = Vector4(18, 10, 2, 1);
    ////r0.transform.forward = Vector4(0, -1, 1, 0);
    ////r0.transform.up = Vector4(0, 1, 1, 0);
    ////r0.transform.right = Vector4(1, 0, 0, 0);


    //r0.vertexs[0] = Vector4(0, 0, 0, 1);
    //r0.vertexs[1] = Vector4(10, 0, 0, 1);
    //r0.vertexs[2] = Vector4(10, 10, 0, 1);
    //r0.vertexs[3] = Vector4(0, 10, 0, 1);

    Reactangular reacs[6];
    reacs[0].transform.forward = Vector4(0, 0, -1, 0);
    reacs[0].transform.up = Vector4(0, 1, 0, 0);
    reacs[0].transform.right = Vector4(-1, 0, 0, 0);

    reacs[1].transform.forward = Vector4(0, 1, 0, 0);
    reacs[1].transform.up = Vector4(0, 0, 1, 0);
    reacs[1].transform.right = Vector4(-1, 0, 0, 0);

    reacs[2].transform.forward = Vector4(-1, 0, 0, 0);
    reacs[2].transform.up = Vector4(0, 1, 0, 0);
    reacs[2].transform.right = Vector4(0, 0, 1, 0);

    reacs[3].transform.position.vec[1] += 10;
    reacs[3].transform.position.vec[0] -= 10;
    reacs[3].transform.forward = Vector4(1, 0, 0, 0);
    reacs[3].transform.up = Vector4(0, -1, 0, 0);
    reacs[3].transform.right = Vector4(0, 0, 1, 0);

    reacs[4].transform.position.vec[0] -= 10;
    reacs[4].transform.position.vec[1] += 10;
    reacs[4].transform.forward = Vector4(0, -1, 0, 0);
    reacs[4].transform.up = Vector4(0, 0, 1, 0);
    reacs[4].transform.right = Vector4(1, 0, 0, 0);

    reacs[5].transform.position.vec[2] += 10;
    reacs[5].transform.position.vec[0] -= 10;
    reacs[5].transform.forward = Vector4(0, 0, 1, 0);
    reacs[5].transform.up = Vector4(0, 1, 0, 0);
    reacs[5].transform.right = Vector4(1, 0, 0, 0);

    /*
    // 第二个面
    Reactangular r1 = r0;
    r1.transform.position = Vector4(28, 10, 2, 1);
    r1.transform.forward = Vector4(0, 1, 0, 0);
    r1.transform.up = Vector4(0, 0, 1, 0);
    r1.transform.right = Vector4(-1, 0, 0, 0);

    Reactangular r2 = r0;
    r2.transform.position = Vector4(18, 10, 2, 1);
    r2.transform.forward = Vector4(-1,0,0, 0);
    r2.transform.up = Vector4(0, 1, 0, 0);
    r2.transform.right = Vector4(0, 0, 1, 0);

    Reactangular r3 = r0;
    r3.transform.position = Vector4(28, 10, 2, 1);
    r3.transform.forward = Vector4(1, 0, 0, 0);
    r3.transform.up = Vector4(0, 1, 0, 0);
    r3.transform.right = Vector4(0, 0, 1, 0);
    */

    //// 平移
    //for (size_t i = 0; i < 4; i++)
    //{
    //    for (size_t j = 0; j < 3; j++)
    //    {
    //        r0.vertexs[i].vec[j] += r0.transform.position.vec[j];
    //    }
    //}

    // 世界变换
    POINT Pt[4 * 6];

    for (size_t i = 0; i < 6; i++)
    {
        WT(reacs[i]);
        get_Pt(Pt + 4 * i, reacs[i]);

        float r = 0;
        float * vec = reacs[i].transform.forward.vec;
        r = vec[0] + vec[1] - vec[2];
        if (r>0)
            Polygon(hDC, Pt + 4 * i, 4);
        Sleep(100);
    }

    // 投影
    //POINT Pt[4];
    //POINT Pt1[4];
    //POINT Pt2[4];
    //POINT Pt3[4];
    //get_Pt(Pt, r0);
    //get_Pt(Pt1, r1);
    //get_Pt(Pt2, r2);
    //get_Pt(Pt3, r3);

    
    
    Sleep(1000);
    exit(0);
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
