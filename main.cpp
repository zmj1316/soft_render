#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <string>   
#include <vector>
#include <algorithm>

using namespace std;

#include "Cube.h"
#include "Zbuffer.h"
#include <math.h>
#include <omp.h>

//����
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

char thread_count = 4;				// openmp thread count

char map_name[] = "map1.bmp";		// bitmap
static int sp_ = 2;					// specular
static float delta = 1e-4;			// delta

int map_width = -1;			//	size of bitmap
int map_height = -1;

// ������̬
int a_rotate = 70;
int a_x = -10;
int a_y = 1;
int a_z = 27;

// hack
float light_mod = 0;

// double buffer
unsigned int* buffer;

static unsigned char* map;
// timer
LARGE_INTEGER t0, t1, t2, tf;
// GDI
HDC hDC;
HDC Memhdc;
HBITMAP Membitmap;
HBITMAP now_bitmap;

// reactangulars
Reactangular reacs[6];
//----------��������---------------
void init(HWND hWnd, HINSTANCE hInstance);


//-----�ص�����-----------------
LRESULT CALLBACK WindProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// read bitmap
// û�д������������Ҫ4��������
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

// from tutorial
// ��Ϊ�ǰ��ֽڷֵ������޸���
void getBilinearFilteredPixelColor(unsigned char tex[], double u, double v,unsigned char res[])
{
	u *= map_width;
	v *= map_height;
	int x = u;
	int y = v;
	double u_ratio = u - x;
	double v_ratio = v - y;
	double u_opposite = 1 - u_ratio;
	double v_opposite = 1 - v_ratio;
	x = min(x, map_width-2);
	y = min(y, map_height-2);
	x = max(0, x);
	y = max(0, y);
	//res[0] = tex[(x + y*map_height) * 3];
	//res[1] = tex[(x + y*map_height) * 3 + 1];
	//res[2] = tex[(x + y*map_height) * 3 + 2];
	res[0] = (tex[(x + y*map_height) * 3] * u_opposite + tex[(x + 1 + y*map_height) * 3] * u_ratio) * v_opposite + (tex[(x + (y + 1)*map_height) * 3] * u_opposite + tex[(x + 1 + (y + 1)*map_height) * 3] * u_ratio) * v_ratio;
	res[1] = (tex[(x + y*map_height) * 3 + 1] * u_opposite + tex[(x + 1 + y*map_height) * 3 + 1] * u_ratio) * v_opposite + (tex[(x + (y + 1)*map_height) * 3 + 1] * u_opposite + tex[(x + 1 + (y + 1)*map_height) * 3 + 1] * u_ratio) * v_ratio;
	res[2] = (tex[(x + y*map_height) * 3 + 2] * u_opposite + tex[(x + 1 + y*map_height) * 3 + 2] * u_ratio) * v_opposite + (tex[(x + (y + 1)*map_height) * 3 + 2] * u_opposite + tex[(x + 1 + (y + 1)*map_height) * 3 + 2] * u_ratio) * v_ratio;
}

// �任����
void transform(float* vec_src, float* vec_dst, Transform t)
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

// ����任
void WT(Reactangular& r0)
{
	for (size_t i = 0; i < 4; i++)
	{
		float* vec = r0.vertexs[i].vec;
		transform(vec, r0.vertexs_world[i].vec, r0.transform);
	}
}

// �ӽǱ任
void VT(Reactangular& r, Transform t)
{
	for (size_t i = 0; i < 4; i++)
	{
		float* vec = r.vertexs_world[i].vec;
		float* dst = r.vertexs_view[i].vec;
		transform(vec, dst, t);
		// ͸���ӽ� �������޸�
		dst[0] /= tan(3.14 / 4);
		dst[1] /= tan(3.14 / 4);
		dst[2] *= 0.8;
		// �������
		Vector4 t0 = r.vertexs_view[1] - r.vertexs_view[0];
		Vector4 t1 = r.vertexs_view[2] - r.vertexs_view[0];
		r.transform.forward = t0 / t1;
	}
}

// ͸��ͶӰ
void get_Pt(POINT* Pt, Reactangular& r0)
{
	for (size_t i = 0; i < 4; i++)
	{
		Pt[i].x = WINDOW_WIDTH/2 + 10 * r0.vertexs_view[i].vec[0] * 50 / r0.vertexs_view[i].vec[2];
		Pt[i].y = WINDOW_HEIGHT/2 - 10 * r0.vertexs_view[i].vec[1] * 50 / r0.vertexs_view[i].vec[2];
	}
}

// RGB ����
int do_RGB(int R, int G, int B)
{
	R = min(255, R);
	G = min(255, G);
	B = min(255, B);
	return R << 16 | G << 8 | B;
}

// ��������
float my_pow(float a, int n)
{
	for (size_t i = 0; i < n; i++)
	{
		a = a * a;
	}
	return a;
}

static int f(int a, int b, int x, int y, POINT p[])
{
	return (p[a].y - p[b].y) * (x - p[b].x) + (p[b].x - p[a].x) * (y - p[b].y);
}

// ��դ��
// ����ֱ�Ӵ��������ν�������˷ָ��������������
int check_point(int x, int y, POINT p[], Vector4* vecs, Vector4& light_spot, Vector4& forward, Vector4 view_pos)
{
	// ��ɫ
	unsigned char color[3];
	// ��������
	double a = double(f(1, 2, x, y, p)) / f(1, 2, p[0].x, p[0].y, p);
	double b = double(f(2, 0, x, y, p)) / f(2, 0, p[1].x, p[1].y, p);
	//float c = float(f(0, 1, x, y, p)) / f(0, 1, p[2].x, p[2].y, p);
	double c = 1 - a - b;
	// �ж��Ƿ����ڲ�
	if (a >= 0 - delta&& a <= 1 + delta && b >= 0 - delta && b <= 1 + delta && c >= 0 - delta&& c <= 1 + delta)
	{
		// �������
		Vector4 point = a * vecs[0] + b * vecs[1] + c * vecs[2];
		// ����
		Vector4 light = point - light_spot;
		// �ý��Ƽ��ټ���
		// ������
		volatile float diff = light * forward * light_mod * forward.mod();
		diff = max(0, diff);
		// ȫ�ֹ�
		diff += 0.1;
		// ����
		// mod()Ϊȡ���ȵ���
		Vector4 reflect = 2.0 * forward.mod() * diff * (forward) - light_mod * light;
		// �ӽǷ���
		Vector4 view_dir = point - view_pos;
		float spec = my_pow(max(0, reflect*view_dir*view_dir.mod()*reflect.mod()), sp_);

		// ͸�ӽ���
		float zr = a / vecs[0].vec[2] + b / vecs[1].vec[2] + c / vecs[2].vec[2];
		double u = ((a*(0 / vecs[2].vec[2]) + b*(0 / vecs[1].vec[2]) + c*(1 / vecs[2].vec[2])) / zr);
		double v = ((a*(0 / vecs[2].vec[2]) + b*(1 / vecs[1].vec[2]) + c*(1 / vecs[2].vec[2])) / zr);

		// �������
		getBilinearFilteredPixelColor(map, u, v,color);
		return do_RGB((diff)* (color[0]) + ((spec)* (0xFF)),
			(diff)* (color[1]) + ((spec)* (0xFF)),
			(diff)* (color[2]) + ((spec)* (0xFF)));
	}
	// ������һ����������ͬ������
	else
	{
		a = double(f(3, 0, x, y, p)) / f(3, 0, p[2].x, p[2].y, p);
		b = double(f(0, 2, x, y, p)) / f(0, 2, p[3].x, p[3].y, p);
		c = 1 - a - b;
		if (a >= 0 - delta&& a <= 1 + delta && b >= 0 - delta && b <= 1 + delta && c >= 0 - delta&& c <= 1 + delta)
		{
			Vector4 point = a * vecs[2] + b * vecs[3] + c * vecs[0];
			Vector4 light = point - light_spot;
			float diff = light * forward * light_mod * forward.mod();
			diff = max(0, diff);
			diff += 0.1;
			Vector4 reflect = 2.0 * forward.mod() * diff * (forward) - light_mod * light;
			Vector4 view_dir = point - view_pos;
			float spec = my_pow(max(0, reflect*view_dir*view_dir.mod()*reflect.mod()), sp_);

			float zr = a / vecs[2].vec[2] + b / vecs[3].vec[2] + c / vecs[0].vec[2];
			double u = ((a*(1 / vecs[2].vec[2]) + b*(1 / vecs[3].vec[2]) + c*(0 / vecs[0].vec[2])) / zr);
			double v = ((a*(1 / vecs[2].vec[2]) + b*(0 / vecs[3].vec[2]) + c*(0 / vecs[0].vec[2])) / zr);
			getBilinearFilteredPixelColor(map, u, v, color);
			return do_RGB((diff)* (color[0]) + ((spec)* (0xFF)),
				(diff)* (color[1]) + ((spec)* (0xFF)),
				(diff)* (color[2]) + ((spec)* (0xFF)));
		}
	}
	// ���ⲿ
	return -1;
}

void init(HWND hWnd, HINSTANCE hInstance)
{
	map = readBMP(map_name);
	hDC = GetDC(hWnd);
	Memhdc = CreateCompatibleDC(hDC);
	Membitmap = CreateCompatibleBitmap(hDC, WINDOW_WIDTH, WINDOW_HEIGHT);
	SelectObject(Memhdc, Membitmap);
	BITMAPINFO bmp_info;
	bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);//�ṹ����ֽ���
	bmp_info.bmiHeader.biWidth = WINDOW_WIDTH;//������Ϊ��λ��λͼ��
	bmp_info.bmiHeader.biHeight = -WINDOW_HEIGHT;//������Ϊ��λ��λͼ��,��Ϊ������ʾ�����Ͻ�Ϊԭ�㣬���������½�Ϊԭ��
	bmp_info.bmiHeader.biPlanes = 1;//Ŀ���豸��ƽ��������������Ϊ1
	bmp_info.bmiHeader.biBitCount = 32; //λͼ��ÿ�����ص�λ��
	bmp_info.bmiHeader.biCompression = BI_RGB;
	bmp_info.bmiHeader.biSizeImage = 0;
	bmp_info.bmiHeader.biXPelsPerMeter = 0;
	bmp_info.bmiHeader.biYPelsPerMeter = 0;
	bmp_info.bmiHeader.biClrUsed = 0;
	bmp_info.bmiHeader.biClrImportant = 0;

	buffer = (unsigned int *)malloc(sizeof(int) * WINDOW_HEIGHT * WINDOW_WIDTH);

	now_bitmap = CreateDIBSection(Memhdc, &bmp_info, DIB_RGB_COLORS, (void**)&buffer, NULL, 0);

	// ��ȷ��ʱ
	QueryPerformanceFrequency(&tf);
	QueryPerformanceCounter(&t0);

	// ��ʼ��������
	reacs[0].transform.forward = Vector4(0, 0, -1, 0);
	reacs[0].transform.up = Vector4(0, 1, 0, 0);
	reacs[0].transform.right = Vector4(-1, 0, 0, 0);

	reacs[1].transform.forward = Vector4(0, 1, 0, 0);
	reacs[1].transform.up = Vector4(0, 0, 1, 0);
	reacs[1].transform.right = Vector4(-1, 0, 0, 0);


	reacs[2].transform.forward = Vector4(-1, 0, 0, 0);
	reacs[2].transform.up = Vector4(0, 1, 0, 0);
	reacs[2].transform.right = Vector4(0, 0, 1, 0);

	reacs[3].transform.position.vec[0] -= SIZE;
	reacs[3].transform.position.vec[2] += SIZE;
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
}

void GameLoop(HWND hwnd)
{
	static POINT Pt[4 * 6];
	static Transform camera;
	static Zbuffer buffers[6];
	static int count = 0;

	// ��ʼ��ʱ
	QueryPerformanceCounter(&t1);

	// ����ͷ
	camera.position = Vector4( -30- a_x, -4 + a_y, 50 - a_z, 1);
	camera.forward = Vector4(0, -1 * cos(3.14 * a_rotate / 100), -1 * sin(3.14 * -a_rotate / 100), 0);
	camera.up = Vector4(0, 1 * sin(-3.14 * a_rotate / 100), -1 * cos(3.14 * a_rotate / 100), 0);
	camera.right = Vector4(-1, 0, 0, 0);
	//camera.forward = Vector4(0, 0, -1, 0);
	//camera.up = Vector4(0, 1, 0, 0);
	//camera.right = Vector4(-1, 0, 0, 0);
	// ���ո�������ͷ
	Vector4 light(10, 0, -200, 0);
	// ȡ������ע�ͣ���������������
	//transform(light.vec, light.vec, camera); 

	// ��ÿ����
	for (int i = 0; i < 6; i++)
	{
		// ��������
		WT(reacs[i]);
		// �ӽ�����
		VT(reacs[i], camera);
		//������˳����
		if (i != 0 && i != 5)
		{
			Vector4 tmp = reacs[i].vertexs_view[1];
			reacs[i].vertexs_view[1] = reacs[i].vertexs_view[3];
			reacs[i].vertexs_view[3] = tmp;
			tmp = reacs[i].vertexs_world[1];
			reacs[i].vertexs_world[1] = reacs[i].vertexs_world[3];
			reacs[i].vertexs_world[3] = tmp;
		}
		// ����ͶӰ
		get_Pt(Pt + 4 * i, reacs[i]);
		// �����淨����
		if (i == 0 || i == 5)
			reacs[i].transform.forward = -reacs[i].transform.forward;
		;
		// �ӽǷ���
		Vector4 view = camera.position - reacs[i].get_center();
		// ???
		//float* view_vec = view.vec;
		//view_vec[0] /= tan(3.14 / 6);
		//view_vec[1] /= tan(3.14 / 6);
		//view_vec[2] *= 0.9;
		float r = reacs[i].transform.forward * view;
		buffers[i].i = i;
		if (r > 0)
		{
			buffers[i].z = reacs[i].get_z();
		}
		else
		{
			buffers[i].z = -1;
		}
	}


	// zsort �Ծ��θ����������
	vector<Zbuffer> bsort(6);
	bsort.assign(buffers, buffers + 6);

	sort(bsort.begin(), bsort.end(), [](const Zbuffer& x, const Zbuffer& y)
	     {
		     return x.z > y.z;
	     });
	light_mod = light.mod();
	for (Zbuffer z : bsort)
	{
		if (z.z < 0)
			break;
		POINT* p = Pt + 4 * z.i;
		int minx = WINDOW_WIDTH, miny = WINDOW_HEIGHT, maxx = 0, maxy = 0;
		POINT p2[4];


		// ���������ͶӰ�ı��εľ���
		for (int i = 0; i < 4; ++i)
		{
			POINT* pp = p + i;
			if (minx > pp->x) minx = pp->x;
			if (miny > pp->y) miny = pp->y;
			if (maxx < pp->x) maxx = pp->x;
			if (maxy < pp->y) maxy = pp->y;
		}
		maxx = min(WINDOW_WIDTH, maxx);
		maxy = min(WINDOW_HEIGHT, maxy);
		minx = max(0, minx);
		miny = max(0, miny);
		// ɨ��
		int i, j;
		for (i = minx; i < maxx; i++)
		{
			// ���ӵĻ����ܸ���
			#pragma omp parallel for
			for (j = miny; j < maxy; j++)
			{
				volatile int color = check_point(i, j, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward, camera.forward);
				if (color >= 0)
				{
					*(buffer + j * WINDOW_WIDTH + i) = color;
				}
			}
		}
	}
	count += 1;
	QueryPerformanceCounter(&t2);

	static char tmp[100] = "FPS:\0";
	//ͨ����֡��ʱ����FPS
	int frames = tf.QuadPart / (t2.QuadPart - t1.QuadPart);
	if (count == 1)
	{
		sprintf(tmp, "FPS: %d\0", frames);
		// ÿ 70 ֡����һ��
		count = -70;
	}
	// �������ֱ���ɫ
	SetBkColor(Memhdc, RGB(0, 0, 0));
	// ����������ɫ
	SetTextColor(Memhdc, RGB(255, 255, 255));
	TextOut(Memhdc, 20, 450, tmp, strlen(tmp));
	// double buffer
	SelectObject(Memhdc, now_bitmap);
	BitBlt(hDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Memhdc, 0, 0, SRCCOPY);


	memset(buffer, 0x00, sizeof(int) * WINDOW_WIDTH * WINDOW_HEIGHT);
}

LRESULT CALLBACK WindProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT Ps;
	HBRUSH NewBrush;
	RECT r;
	POINT Pt[3];

	switch (Msg)
	{
	case WM_COMMAND:
		break;
	// ��������
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			a_x++;
			break;
		case VK_RIGHT:
			a_x--;
			break;
		case VK_UP:
			a_y++;
			break;
		case VK_DOWN:
			a_y--;
			break;
		case VK_F1:
			a_rotate++;
			break;
		case VK_F2:
			a_rotate--;
			break;
		case VK_F3:
			a_z--;
			break;
		case VK_F4:
			a_z++;
			break;
		}
		// ���¼���
		//light_mod = 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX WndCls;
	omp_set_num_threads(thread_count);

	MSG Msg;
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

	HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
	                                                 TEXT("MAIN"), TEXT("TEST"),
	                                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                                                 CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
	                                                 NULL, NULL, hInstance, NULL);
	init(hWnd, hInstance);
	UpdateWindow(hWnd);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		GameLoop(hWnd);
	}

	return static_cast<int>(msg.wParam);
}
