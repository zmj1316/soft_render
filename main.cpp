#define _CRT_SECURE_NO_WARNINGS
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

char map_name[] = "map.bmp";

unsigned int colors[] = { 0x00F0F0E0,0x00FF0000,0x0000FF00,0x000000FF,0x00FFFF00,0x0000FFFF };
unsigned char RGBs[] = { 0x2F,0x3F,0x5F,0x7F,0x9F,0xbF,0xdF,0xfF };
char thread_count = 4;
static int frame_count = 1000;
static int map_width = 0;
static int map_height = 0;
//----------函数声明---------------
void init(HWND hWnd, HINSTANCE hInstance);


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

unsigned char* readBMP(char* filename)
{
    int i;
    FILE* f = fopen(filename, "rb");
    if (f == NULL)
        return NULL;
    unsigned char info[54];
    fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

    // extract image height and width from header
    int width = *(int*)&info[18];
    int height = *(int*)&info[22];
    map_width = width;
    map_height = height;
    int size = 3 * width * height;
    unsigned char* data = new unsigned char[size]; // allocate 3 bytes per pixel
    fread(data, sizeof(unsigned char), size, f); // read the rest of the data at once
    fclose(f);

    for (i = 0; i < size; i += 3)
    {
        unsigned char tmp = data[i];
        data[i] = data[i + 2];
        data[i + 2] = tmp;
    }

    return data;
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
        Vector4 t0 = r.vertexs_view[1] - r.vertexs_view[0];
        Vector4 t1 = r.vertexs_view[2] - r.vertexs_view[0];
        r.transform.forward = t0 / t1;
    }
}

void get_Pt(POINT* Pt, Reactangular& r0)
{
    for (size_t i = 0; i < 4; i++)
    {
        Pt[i].x = -100 + 10 * r0.vertexs_view[i].vec[0] * 50 / r0.vertexs_view[i].vec[2];
        Pt[i].y = 200 - 10 * r0.vertexs_view[i].vec[1] * 50 / r0.vertexs_view[i].vec[2];
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

int do_RGB(int R, int G, int B)
{
    R = min(255, R);
    G = min(255, G);
    B = min(255, B);
    return R << 16 | G << 8 | B;
}

int do_RGB(int color, float diff)
{
    return do_RGB((color >> 16) * diff, (color >> 8) * diff, color*diff);
}

float my_pow(float a, int n)
{
    for (size_t i = 0; i < n; i++)
    {
        a = a*a;
    }
    return a;
}

int check_point(int x, int y, POINT p[], Vector4 * vecs,Vector4& light_spot,Vector4 & forward, Vector4 view_pos)
{
    static unsigned char * map = readBMP(map_name);
    float a = float(f(1, 2, x, y, p)) / f(1, 2, p[0].x, p[0].y, p);
    float b = float(f(2, 0, x, y, p)) / f(2, 0, p[1].x, p[1].y, p);
	//float c = float(f(0, 1, x, y, p)) / f(0, 1, p[2].x, p[2].y, p);
    float c = 1 - a - b;
    if (a >= 0 && a<=1 && b>=0 && b<=1 && c>=0 && c <= 1)
    {
		Vector4 point = a*vecs[0] + b*vecs[1] + c*vecs[2];
		Vector4 light = point - light_spot;
        float diff = light.normal()*forward.normal();
        diff = max(0, diff);
        diff += 0.1;
        Vector4 relfect = 2.0*diff * (forward.normal()) - light.normal();
        relfect = relfect.normal();
        double spec = my_pow(max(0, relfect*(point - view_pos).normal()), 2);
        float zr = a / vecs[0].vec[2] + b / vecs[1].vec[2] + c / vecs[2].vec[2];
        int u = ((a*(0 / vecs[2].vec[2]) + b*(0 / vecs[1].vec[2]) + c*(1 / vecs[2].vec[2])) / zr) * map_width;
        int v = ((a*(0 / vecs[2].vec[2]) + b*(1 / vecs[1].vec[2]) + c*(1 / vecs[2].vec[2])) / zr) * map_height;
        return do_RGB((diff)* (0xFF & ((map[(u + v*map_width) * 3]))) + ((spec)* (0xFF)),
            (diff)* (0xFF & ((map[(u + v * map_width) * 3 + 1]))) + ((spec)* (0xFF)),
            (diff)* (0xFF & ((map[(u + v * map_width) * 3 + 2]))) + ((spec)* (0xFF)));
	}
	else
    {
        a = float(f(3, 0, x, y, p)) / f(3, 0, p[2].x, p[2].y, p);
        b = float(f(0, 2, x, y, p)) / f(0, 2, p[3].x, p[3].y, p);
        c = 1 - a - b;
        if (a >= 0 && a<=1 && b>=0 && b<=1 && c>=0 && c <= 1)
        {
			Vector4 point = a*vecs[2] + b*vecs[3] + c*vecs[0];
			Vector4 light = point - light_spot;

			float diff = light.normal()*forward.normal();
            diff = max(0, diff);
            diff += 0.1;
			Vector4 relfect = 2.0*diff * (forward.normal()) - light.normal();
			relfect = relfect.normal();
            float spec = my_pow(max(0, relfect*(point - view_pos).normal()), 2);

            //float spec = max(0, relfect*(point - view_pos).normal());
            ////float spec2 = spec;
            //unsigned char *trick = (unsigned char *)&spec;
            ////int exp = trick[7] & 0xEF << 4 | (trick[6] >> 4);
            //spec = pow(spec, 200);
            //exp = trick[7] & 0xEF << 4 | (trick[6] >> 4);
            //int tmp0 = trick[7];
            //int tmp1 = trick[6];
            //trick[7] -=8;
            //trick[7] &= 0xDF;
            float zr = a / vecs[2].vec[2] + b / vecs[3].vec[2] + c / vecs[0].vec[2];
            int u = ((a*(1 / vecs[2].vec[2]) + b*(1 / vecs[3].vec[2]) + c*(0 / vecs[0].vec[2])) / zr) * map_width;
            int v = ((a*(1 / vecs[2].vec[2]) + b*(0 / vecs[3].vec[2]) + c*(0 / vecs[0].vec[2])) / zr) * map_height;
            return do_RGB((diff)* (0xFF & ((map[(u + v * map_width) * 3]))) + ((spec)* (0xFF)),
                (diff)* (0xFF & ((map[(u + v * map_width) * 3 + 1]))) + ((spec)* (0xFF)),
                (diff)* (0xFF & ((map[(u + v * map_width) * 3 + 2]))) + ((spec)* (0xFF)));
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
	
	QueryPerformanceCounter(&t1);

    camera.position = Vector4(-2 * cos(3.14*count / 500), -2 * cos(3.14*count / 500), 80 - 20 * sin(3.14*count / 500), 1);
	camera.forward = Vector4(0, -1 * cos(3.14*count / 500), -1 * sin(3.14*-count / 500), 0);
	camera.up = Vector4(0, 1 * sin(-3.14*count / 500), -1 * cos(3.14*count / 500), 0);
	//camera.forward = Vector4(0, -0.7, -0.7, 0);
	//camera.up = Vector4(0, 0.7, -0.7, 0);
	camera.right = Vector4(-1, 0, 0, 0);
	Vector4 light(100, 20, -20, 0);
	//transform(light.vec, light.vec, camera);



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

		int i, j;


        for (i = minx ; i < maxx; i++)
        {
#pragma omp parallel for
            for (j = miny; j < maxy; j++)
            {
				int color;
				color = check_point(i, j, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward,camera.forward);
				if(color>=0)
                {
					*(buffer + j*WINDOW_WIDTH + i) = color;
                }

            }
        }
    }
    count+=1;
    if (count > frame_count)
    {
		break;
    }
    else 
    {
        QueryPerformanceCounter(&t2);

        char tmp[100];
        int frames = tf.QuadPart / (t2.QuadPart - t1.QuadPart);
        sprintf(tmp, "FPS: %d\0", frames);
        // 设置文字背景色
        SetBkColor(Memhdc, RGB(0, 0, 0));
        // 设置文字颜色
        SetTextColor(Memhdc, RGB(255, 255, 255));
        TextOut(Memhdc, 20, 550, tmp, strlen(tmp));

		SelectObject(Memhdc, now_bitmap);
		BitBlt(hDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Memhdc, 0, 0, SRCCOPY);


        memset(buffer, 0x00, sizeof(int)*WINDOW_WIDTH*WINDOW_HEIGHT);

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

	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}
