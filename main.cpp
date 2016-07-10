#include <windows.h>
#include   <string>   
#include <vector>
#include <algorithm>

using   namespace   std;

#include "Cube.h"
#include "matrix.h"
#include "Zbuffer.h"
#include <math.h>
#include <omp.h>
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

unsigned int colors[] = { 0x00F0F0E0,0x00FF0000,0x0000FF00,0x000000FF,0x00FFFF00,0x0000FFFF };
unsigned char RGBs[] = { 0x1F,0x3F,0x5F,0x7F,0x9F,0xbF,0xdF,0xfF };
char thread_count = 2;

//----------函数声明---------------
void init(HWND hWnd, HINSTANCE hInstance);
//void pix(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps);//画点,也就是像素输出
//void rect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps);//----画矩形----
//void fillrect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//填充矩形
//void linerect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//线框矩形
//void triangle(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, POINT Pt[3], PAINTSTRUCT &Ps);//三角形
//void ellipse(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps);//椭圆
//void DrawContent(HWND hWnd);

//-----回调函数-----------------
LRESULT CALLBACK WindProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	WNDCLASSEX  WndCls;
	omp_set_num_threads(thread_count);

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
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 800,
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

void transform(float* vec_src, float *vec_dst, Transform t)
{
	float new_vec[4];
	for (size_t i = 0; i < 4; i++)
	{
		new_vec[i] = t.right.vec[i] * vec_src[0]
			+ t.up.vec[i] * vec_src[1]
			+ t.forward.vec[i] * vec_src[2]
			+ t.position.vec[i] * vec_src[3];
	}
	memcpy(vec_dst, new_vec, 4 * sizeof(float));
}

// 视角变换
void VT(Reactangular& r, Transform t)
{
    for (size_t i = 0; i < 4; i++)
    {
        float* vec = r.vertexs_world[i].vec;
		float * dst = r.vertexs_view[i].vec;
		transform(vec, dst, t);
        dst[0] /= tan(3.14 / 6);
        dst[1] /= tan(3.14 / 6);
        dst[2] *= 0.9;
        dst[2] -= 0;
        Vector4 t0 = r.vertexs_view[1] - r.vertexs_view[0];
        Vector4 t1 = r.vertexs_view[2] - r.vertexs_view[0];
        r.transform.forward = t0 / t1;
    }
}

void get_Pt(POINT* Pt, Reactangular& r0)
{
    for (size_t i = 0; i < 4; i++)
    {
        Pt[i].x = 200 + 10 * r0.vertexs_view[i].vec[0] * 50 / r0.vertexs_view[i].vec[2];
        Pt[i].y = 200 - 10 * r0.vertexs_view[i].vec[1] * 50 / r0.vertexs_view[i].vec[2];
        //Pt[i].x = r0.vertexs[i].vec[0] * (50 / r0.vertexs[i].vec[2]);
        //Pt[i].y = r0.vertexs[i].vec[1] * (50 / r0.vertexs[i].vec[2]);
    }
}

struct Line
{
    int x, y; // point
    float rate;
};

struct Line_point
{
    int x, y;
    BYTE flag;
};

int f(int a, int b, int x, int y, POINT p[])
{
	return (p[a].y - p[b].y)*(x-p[b].x) + (p[b].x - p[a].x)*(y - p[b].y);
	//return (p[a].y - p[b].y)*x - (p[b].x - p[a].x)*y + p[a].x*p[b].y - p[b].x*p[a].y;
}


int check_point(int x, int y, POINT p[], Vector4 * vecs,Vector4& light_spot,Vector4 & forward)
{
    float a = float(f(1, 2, x, y, p)) / f(1, 2, p[0].x, p[0].y, p);
    float b = float(f(2, 0, x, y, p)) / f(2, 0, p[1].x, p[1].y, p);
	float c = float(f(0, 1, x, y, p)) / f(0, 1, p[2].x, p[2].y, p);
    if (a >= 0 && a<=1 && b>=0 && b<=1 && c>=0 && c <= 1)
    {
		Vector4 point = a*vecs[0] + b*vecs[1] + c*vecs[2];
		Vector4 light = point - light_spot;
		float diff = light.normal()*forward.normal();
		Vector4 relfect = 2.0*diff * forward - light;
		relfect = relfect.normal();
		float spec = pow(max(0, relfect*point.normal()),2);
		spec *= 1.3;
		spec += 0.05;
		return byte((spec)* (0xFF * b)) << 16 | byte((spec)* (0xFF * c)) << 8 | byte((spec)* (0xFF * a));
	}
	else
    {
        a = float(f(3, 0, x, y, p)) / f(3, 0, p[2].x, p[2].y, p);
        b = float(f(0, 2, x, y, p)) / f(0, 2, p[3].x, p[3].y, p);
        c = float(f(2, 3, x, y, p)) / f(2, 3, p[0].x, p[0].y, p);
        //c = 1 - a - b;
        if (a >= 0 && a<=1 && b>=0 && b<=1 && c>=0 && c <= 1)
        {
			Vector4 point = a*vecs[2] + b*vecs[3] + c*vecs[0];
			Vector4 light = point - light_spot;
			float diff = light.normal()*forward.normal();
			Vector4 relfect = 2.0*diff * forward - light;
			relfect = relfect.normal();
			float spec = pow(max(0, relfect*point.normal()), 2);
			spec *= 1.3;
			spec += 0.05;
			//if (diff < 0)
			//	diff = -diff;
			//return spec;
			return byte((spec) * (0xFF*b)) << 16 | byte((spec) * (0xFF*a)) << 8 | byte((spec)* (0xFF*c));


        }
    }

    return -1;
}

void init(HWND hWnd, HINSTANCE hInstance)
{
    HDC hDC = GetDC(hWnd);
    HDC Memhdc = CreateCompatibleDC(hDC);
    HBITMAP Membitmap = CreateCompatibleBitmap(hDC, 800, 800);
    SelectObject(Memhdc, Membitmap);
	BITMAPINFO  bmp_info;
	bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);//结构体的字节数
	bmp_info.bmiHeader.biWidth = WINDOW_WIDTH;//以像素为单位的位图宽
	bmp_info.bmiHeader.biHeight = -WINDOW_HEIGHT;//以像素为单位的位图高,若为负，表示以左上角为原点，否则以左下角为原点
	bmp_info.bmiHeader.biPlanes = 1;//目标设备的平面数，必须设置为1
	bmp_info.bmiHeader.biBitCount = 32; //位图中每个像素的位数
	bmp_info.bmiHeader.biCompression = BI_RGB;
	bmp_info.bmiHeader.biSizeImage = 0;
	bmp_info.bmiHeader.biXPelsPerMeter = 0;
	bmp_info.bmiHeader.biYPelsPerMeter = 0;
	bmp_info.bmiHeader.biClrUsed = 0;
	bmp_info.bmiHeader.biClrImportant = 0;

	unsigned int *buffer = (unsigned int *)malloc(sizeof(int)*WINDOW_HEIGHT*WINDOW_WIDTH);

	auto now_bitmap = CreateDIBSection(Memhdc, &bmp_info, DIB_RGB_COLORS, (void**)&buffer, NULL, 0);

    int count = 0;
	LARGE_INTEGER t1, t2, tf;
	QueryPerformanceFrequency(&tf);
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
	Zbuffer buffers[6];
while(1)
{
	
	//QueryPerformanceCounter(&t1);

    camera.position = Vector4(5, -5, 40 + 2 * sin(3.14*count / 300), 1);
	camera.forward = Vector4(0, -1 * cos(3.14*count / 300), -1 * sin(3.14*-count / 300), 0);
	camera.up = Vector4(0, 1 * sin(-3.14*count / 300), -1 * cos(3.14*count / 300), 0);
	//camera.forward = Vector4(0, -0.7, -0.7, 0);
	//camera.up = Vector4(0, 0.7, -0.7, 0);
	camera.right = Vector4(-1, 0, 0, 0);
	Vector4 light(30 - 5 * cos(3.14*count / 500), 20-5 * sin(3.14*count / 500), -100, 0);
	transform(light.vec, light.vec, camera);



	#pragma omp parallel for
    for (int i = 0; i < 6; i++)
    {
        WT(reacs[i]);
        VT(reacs[i], camera);
        get_Pt(Pt + 4 * i, reacs[i]);
		if (i == 0 || i == 5)
			reacs[i].transform.forward = -reacs[i].transform.forward;
        float r = 0;
        float * vec = reacs[i].transform.forward.vec;
        Vector4 view = camera.position - reacs[i].get_center();
        float *view_vec = view.vec;
        view_vec[0] /= tan(3.14 / 6);
        view_vec[1] /= tan(3.14 / 6);
        view_vec[2] *= 0.9;
        r = reacs[i].transform.forward * view;
        buffers[i].i = i;
        if (r>0)
        {
            buffers[i].z = reacs[i].get_z();
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
        //SelectObject(Memhdc, burushed[z.i]);
        //Polygon(Memhdc, Pt + 4 * z.i, 4);
		//Vector4 directioin = light - reacs[z.i].transform.forward;
		//float diff = light*reacs[z.i].transform.forward/(reacs[z.i].transform.forward.mod());
		//if (diff > 0) diff = 0;
		//else diff = -diff;
		//diff += 0.2;
        POINT *p = Pt + 4 * z.i;
        int minx = 800, miny = 800, maxx = 0, maxy = 0;
		// sort points
		int max_sum = 0;
		int min_sum = 800;
		int min_p = 0;
		int max_p = 0;
        for (int i = 0; i < 4; ++i)
        {
            POINT *pp = p + i;
            if (minx>pp->x) minx = pp->x;
            if (miny>pp->y) miny = pp->y;
            if (maxx < pp->x) maxx = pp->x;
            if (maxy < pp->y) maxy = pp->y;

			int sum = p[i].x + p[i].y;
			if (sum > max_sum)
			{
				max_sum = sum;
				max_p = i;
			}
			if (sum < min_sum)
			{
				min_sum = sum;
				min_p = i;
			}
        }
		if (maxx > 800) maxx = 800;
		if (maxy > 800) maxy = 800;
		if (minx < 0) minx = 0;
		if (miny < 0) miny = 0;


		POINT sorted_p[4];
		sorted_p[0] = p[min_p];
		sorted_p[2] = p[max_p];
		bool flag_b = true;
		for (size_t i = 0; i < 4; i++)
		{
			if(i!=min_p&&i!=max_p)
			{
				if (flag_b)
					sorted_p[1] = p[i];
				else
					sorted_p[3] = p[i];
				flag_b = false;
			}
		}
		if(sorted_p[1].x<sorted_p[3].x)
		{
			POINT tmp = sorted_p[1];
			sorted_p[1] = sorted_p[3];
			sorted_p[3] = tmp;
		}




		int i, j;
        for (i = minx ; i < maxx; i++)
        {
		#pragma omp parallel for
            for (j = miny; j < maxy; j++)
            {
				float diff;
				int color;
				color = check_point(i, j, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward);
                //if ((diff = check_point(i, j, p,reacs[z.i].vertexs_view,light,reacs[z.i].transform.forward))>=0)
				if(color>=0)
                {
					//diff += 0.1;
					//if (diff > 1) diff = 1;
                    //SetPixel(Memhdc, i, j, RGB(223 * ((z.i & 4) >> 2) + 10, 223 * ((z.i & 2) >> 1) + 10, 223 * (z.i & 1) +10));
					//*(buffer + j*WINDOW_WIDTH + i) = ((int(155 * ((z.i & 4) >> 2) * diff) << 16 | (int(155 * ((z.i & 2) >> 1) * diff) << 8) | (int(155 * (z.i & 1) * diff))));
					//*(buffer + j*WINDOW_WIDTH + i) = byte(diff*RGBs[z.i + 1])<<16 | byte(diff*RGBs[z.i + 2])<<8 | byte(diff*RGBs[z.i - 3]);
					*(buffer + j*WINDOW_WIDTH + i) = color;
                }
            }
        }
    }
    count+=1;
    if (count > 500)
    {
		break;
    }
    else 
    {
        //BitBlt(hDC, 0, 0, 800, 800, Memhdc, 0, 0, SRCCOPY);
        //static auto white_brush = CreateSolidBrush(RGB(255, 255, 255));
        //SelectObject(Memhdc, white_brush);
        //Rectangle(Memhdc, 0, 0, 800, 800);
		SelectObject(Memhdc, now_bitmap);
		BitBlt(hDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Memhdc, 0, 0, SRCCOPY);
		memset(buffer, 0x00, sizeof(int)*WINDOW_WIDTH*WINDOW_HEIGHT);
		//QueryPerformanceCounter(&t2);
		//printf("Lasting Time: %lf us\n", (1000000.0*t2.QuadPart - 1000000.0*t1.QuadPart) / tf.QuadPart);
    }
}
    DeleteObject(Membitmap);
    DeleteDC(Memhdc);
    DeleteDC(hDC);
    EndPaint(hWnd,NULL);

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
	//	if (DWORD(lParam) == int(h))
	//	{
	//		pix(hWnd, hDC, NewBrush, Ps);
	//		break;
	//	}

	//	h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("矩形"));
	//	if (DWORD(lParam) == int(h))
	//	{
	//		rect(hWnd, hDC, NewBrush, Ps);
	//		break;
	//	}

	//	h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("填充矩形"));
	//	if (DWORD(lParam) == int(h))
	//	{
	//		fillrect(hWnd, hDC, NewBrush, r, Ps);
	//		break;
	//	}

	//	h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("线框矩形"));
	//	if (DWORD(lParam) == int(h))
	//	{
	//		linerect(hWnd, hDC, NewBrush, r, Ps);
	//		break;
	//	}

	//	h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("三角形"));
	//	if (DWORD(lParam) == int(h))
	//	{
	//		triangle(hWnd, hDC, NewBrush, Pt, Ps);
	//		break;
	//	}

	//	h = FindWindowEx(hWnd, NULL, TEXT("BUTTON"), TEXT("椭圆"));
	//	if (DWORD(lParam) == int(h))
	//	{
	//		ellipse(hWnd, hDC, NewBrush, r, Ps);
	//		break;
	//	}
	//case WM_PAINT:
	//	DrawContent(hWnd);//为什么加了这一句后开始按钮就会显示，而不加的话按钮需要重绘以后显示
	//	break;

	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}

//
//
////画点
//void pix(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps)
//{
//	//hDC = BeginPaint(hWnd, &Ps);//用BeginPaint的话消息一定要WM_PAINT响应，但是WM_PAINT里面只有DrawContent(hWnd);
//	//所以用getDC
//	hDC = GetDC(hWnd);
//	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
//	//SelectObject(hDC, NewBrush);
//	for (int x = 500; x<600; x++)//由于一个点太小了，所以这里花了100个点构成线段
//		SetPixel(hDC, x, 50, RGB(255, 0, 0));
//	//DeleteObject(NewBrush);
//	//EndPaint(hWnd, &Ps);
//	ReleaseDC(hWnd, hDC);
//}
////矩形
//void rect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, PAINTSTRUCT &Ps)
//{
//	//hDC = BeginPaint(hWnd, &Ps);
//	hDC = GetDC(hWnd);
//	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
//	SelectObject(hDC, NewBrush);
//	Rectangle(hDC, 400, 400, 500, 500);
//	DeleteObject(NewBrush);
//	ReleaseDC(hWnd, hDC);
//	//EndPaint(hWnd, &Ps);
//}
//
////填充矩形
//void fillrect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps)
//{
//	//hDC = BeginPaint(hWnd, &Ps);
//	hDC = GetDC(hWnd);
//	NewBrush = CreateSolidBrush(RGB(25, 25, 5));
//	SelectObject(hDC, NewBrush);
//	SetRect(&r, 200, 200, 250, 250);
//	FillRect(hDC, &r, NewBrush);
//	DeleteObject(NewBrush);
//	//EndPaint(hWnd, &Ps);
//	ReleaseDC(hWnd, hDC);
//}
//
////线框矩形
//void linerect(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps)
//{
//	//hDC = BeginPaint(hWnd, &Ps);
//	hDC = GetDC(hWnd);
//	NewBrush = CreateSolidBrush(RGB(25, 25, 5));
//	SelectObject(hDC, NewBrush);
//	SetRect(&r, 250, 250, 400, 400);
//	FrameRect(hDC, &r, NewBrush);
//	DeleteObject(NewBrush);
//	DeleteObject(NewBrush);
//	//EndPaint(hWnd, &Ps);
//	ReleaseDC(hWnd, hDC);
//}
////三角形
//void triangle(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, POINT Pt[3], PAINTSTRUCT &Ps)
//{
//	//hDC = BeginPaint(hWnd, &Ps);
//	hDC = GetDC(hWnd);
//	NewBrush = CreateSolidBrush(RGB(50, 50, 50));
//	SelectObject(hDC, NewBrush);
//	Pt[0].x = 425; Pt[0].y = 40;
//	Pt[1].x = 395; Pt[1].y = 70;
//	Pt[2].x = 455; Pt[2].y = 70;
//	Polygon(hDC, Pt, 3);
//	DeleteObject(NewBrush);
//
//	NewBrush = CreateSolidBrush(RGB(0, 255, 0));
//	SelectObject(hDC, NewBrush);
//	Pt[0].x = 365; Pt[0].y = 110;
//	Pt[1].x = 395; Pt[1].y = 80;
//	Pt[2].x = 395; Pt[2].y = 140;
//	Polygon(hDC, Pt, 3);
//	DeleteObject(NewBrush);
//
//	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
//	SelectObject(hDC, NewBrush);
//	Pt[0].x = 485; Pt[0].y = 110;
//	Pt[1].x = 455; Pt[1].y = 80;
//	Pt[2].x = 455; Pt[2].y = 140;
//	Polygon(hDC, Pt, 3);
//	DeleteObject(NewBrush);
//
//	NewBrush = CreateSolidBrush(RGB(0, 0, 255));
//	SelectObject(hDC, NewBrush);
//	Pt[0].x = 425; Pt[0].y = 180;
//	Pt[1].x = 455; Pt[1].y = 150;
//	Pt[2].x = 395; Pt[2].y = 150;
//	Polygon(hDC, Pt, 3);
//	DeleteObject(NewBrush);
//	//EndPaint(hWnd, &Ps);
//	ReleaseDC(hWnd, hDC);
//}
//
////椭圆
//void ellipse(HWND &hWnd, HDC &hDC, HBRUSH &NewBrush, RECT &r, PAINTSTRUCT &Ps)//椭圆
//{
//	//hDC = BeginPaint(hWnd, &Ps);
//	hDC = GetDC(hWnd);
//	NewBrush = CreateSolidBrush(RGB(255, 0, 0));
//	SelectObject(hDC, NewBrush);
//	Ellipse(hDC, 500, 500, 600, 600);
//	DeleteObject(NewBrush);
//	//EndPaint(hWnd, &Ps);
//	ReleaseDC(hWnd, hDC);
//}
//
//void DrawContent(HWND hWnd)
//{
//	HDC         hDC;
//	PAINTSTRUCT Ps;
//	hDC = BeginPaint(hWnd, &Ps);//用BeginPaint的话消息一定要WM_Paint响应
//	EndPaint(hWnd, &Ps);
//
//}
