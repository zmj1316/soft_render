#include <windows.h>
#include   <string>   
#include <vector>
#include <algorithm>

using   namespace   std;

#include "Cube.h"
#include "matrix.h"
#include "Zbuffer.h"
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
        memcpy(r0.vertexs_world[i].vec, new_vec, 4 * sizeof(float));
    }
}

// 视角变换
void VT(Reactangular& r, Transform t)
{
    for (size_t i = 0; i < 4; i++)
    {
        float* vec = r.vertexs_world[i].vec;
        float new_vec[4];

        matrix<float> m(4, 4);
        for (size_t i = 0; i < 4; i++)
        {
            m.setvalue(i, 0, t.right.vec[i]);
            m.setvalue(i, 1, t.up.vec[i]);
            m.setvalue(i, 2, t.forward.vec[i]);
            m.setvalue(i, 3, t.position.vec[i]);
        }
        m.invert();

        for (size_t i = 0; i < 4; i++)
        {
            new_vec[i] = t.right.vec[i] * vec[0]
                + t.up.vec[i] * vec[1]
                + t.forward.vec[i] * vec[2]
                + t.position.vec[i] * vec[3];
            //float right, up, forward, position;
            //bool res;
            //m.getvalue(i, 0, right, res);
            //m.getvalue(i, 1, up, res);
            //m.getvalue(i, 2, forward, res);
            //m.getvalue(i, 3, position, res);
            //new_vec[i] = right*vec[0]
            //    + up*vec[1]
            //    + forward*vec[2]
            //    + position*vec[3];
        }
        new_vec[0] /= tan(3.14 / 6);
        new_vec[1] /= tan(3.14 / 6);
        new_vec[2] *= 0.9;
        new_vec[2] -= 0;
        memcpy(r.vertexs_view[i].vec, new_vec, 4 * sizeof(float));
        Vector4 t0 = r.vertexs_view[1] - r.vertexs_view[0];
        Vector4 t1 = r.vertexs_view[2] - r.vertexs_view[0];
        r.transform.forward = t0 / t1;
    }
}

void get_Pt(POINT* Pt, Reactangular& r0)
{
    for (size_t i = 0; i < 4; i++)
    {
        Pt[i].x = 400 + 10 * r0.vertexs_view[i].vec[0] * 50 / r0.vertexs_view[i].vec[2];
        Pt[i].y = 400 - 10 * r0.vertexs_view[i].vec[1] * 50 / r0.vertexs_view[i].vec[2];
        //Pt[i].x = r0.vertexs[i].vec[0] * (50 / r0.vertexs[i].vec[2]);
        //Pt[i].y = r0.vertexs[i].vec[1] * (50 / r0.vertexs[i].vec[2]);
    }
}

void init(HWND hWnd, HINSTANCE hInstance)
{
    HDC hDC = GetDC(hWnd);

    int count = 99;
_LOOP:


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

    reacs[3].transform.position.vec[1] += SIZE;
    reacs[3].transform.position.vec[0] -= SIZE;
    reacs[3].transform.forward = Vector4(1, 0, 0, 0);
    reacs[3].transform.up = Vector4(0, 1, 0, 0);
    reacs[3].transform.right = Vector4(0, 0, -1, 0);

    reacs[4].transform.position.vec[0] -= SIZE;
    reacs[4].transform.position.vec[1] += SIZE;
    reacs[4].transform.forward = Vector4(0, -1, 0, 0);
    reacs[4].transform.up = Vector4(0, 0, 1, 0);
    reacs[4].transform.right = Vector4(1, 0, 0, 0);

    reacs[5].transform.position.vec[2] += SIZE;
    reacs[5].transform.position.vec[0] -= SIZE;
    reacs[5].transform.forward = Vector4(0, 0, 1, 0);
    reacs[5].transform.up = Vector4(0, 1, 0, 0);
    reacs[5].transform.right = Vector4(1, 0, 0, 0);


    // 世界变换
    POINT Pt[4 * 6];
    Transform camera;
    camera.position = Vector4(0, -10, 40, 1);
    camera.forward = Vector4(0, -1 * cos(3.14*count / 100), -1 * sin(3.14*count / 100), 0);
    camera.up = Vector4(0, 1 * sin(3.14*count / 100), -1 * cos(3.14*count / 100), 0);
    camera.right = Vector4(-1, 0, 0, 0);

    static HBRUSH burushed[6];
    static Zbuffer buffers[6];
    for (int i = 0; i < 6; ++i)
    {
        burushed[i] = CreateSolidBrush(RGB(255*((i&4)>>2), 255*((i&2)>>1), 255*(i&1)));
    }
    burushed[0] = CreateSolidBrush(RGB(100, 200, 250));
    for (size_t i = 0; i < 6; i++)
    {
        WT(reacs[i]);
        VT(reacs[i], camera);
        get_Pt(Pt + 4 * i, reacs[i]);

        float r = 0;
        float * vec = reacs[i].transform.forward.vec;
        Vector4 view = camera.position - reacs[i].get_center();
        float *view_vec = view.vec;
        //view.vec[1] += 65;
        view_vec[0] /= tan(3.14 / 6);
        view_vec[1] /= tan(3.14 / 6);
        view_vec[2] *= 0.9;
        //r = view * reacs[i].transform.forward;
        //r = vec[0] * camera.position.vec[0] + vec[1] * camera.position.vec[1] + vec[2] * camera.position.vec[2];
        r = reacs[i].transform.forward * view;
        //r = vec[0] + vec[1] + vec[2];
        buffers[i].i = i;
        if (r<0&&(i==0||i==5)||(r>0&&(i!=0&&i!=5)))
        {
            buffers[i].z = reacs[i].get_z();
            //SelectObject(hDC, burushed[i]);
            //Polygon(hDC, Pt + 4 * i, 4);
        }
        else
        {
            buffers[i].z = -1;
        }
    }


    // zsort
    vector<Zbuffer> bsort(6);
    bsort.assign(buffers,buffers+6);
    
    sort(bsort.begin(), bsort.end(), [](const Zbuffer&x, const Zbuffer&y){return x.z > y.z; });
    for (Zbuffer z : bsort)
    {
        if (z.z < 0)
            break;
        SelectObject(hDC, burushed[z.i]);
        Polygon(hDC, Pt + 4 * z.i, 4);
    }
    Sleep(30);
    count+=1;
    if (count > 400)
        exit(0);
    else 
    {
        static auto white_brush = CreateSolidBrush(RGB(255, 255, 255));
        SelectObject(hDC, white_brush);
        Rectangle(hDC, 0, 0, 800, 800);
    }
    goto _LOOP;

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
