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
//参数
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

byte thread_count_b = 100; // openmp thread count

float z_far_f = 1000;
float z_near_f = 0.1;
char map_name_str[] = "map1.bmp"; // bitmap
static int specular_i = 6; // specular 2^6=64
static float delta_f = 1e-5; // delta
int map_width_i = -1; //	size of bitmap
int map_height_i = -1;

//float view_angle = 3.14/180 * 90;
const float cot_f = 3;
// 控制姿态
int a_dep = 0;
int a_x = 0;
int a_y = 0;
int a_z = 0;

// hack
float light_mod = 0;
//float reflect_mod = 0;
// float buffer
unsigned int* buffer;

static float* map;
// timer
LARGE_INTEGER t0, t1, t2, tf;
// GDI
HDC hDC;
HDC Memhdc;
HBITMAP Membitmap;
HBITMAP now_bitmap;

// reactangulars
Reactangular reacs[6];

Transform camera;

typedef struct mPOINT
{
	short x;
	short y;
} mPOINT;

//----------函数声明---------------
void init(HWND hWnd, HINSTANCE hInstance);


//-----回调函数-----------------
LRESULT CALLBACK WindProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// read bitmap
// 没有处理对齐所以需要4的整数倍
float* readBMP(char* filename)
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
	map_width_i = width;
	map_height_i = height;
	int size = 3 * width * height;
	unsigned char* data = new unsigned char[size]; // allocate 3 bytes per pixel
	float* data_f = new float[size * 3];
	fread(data, sizeof(unsigned char), size, f); // read the rest of the data at once
	fclose(f);

	for (i = 0; i < size; i += 3)
	{
		//unsigned char tmp = data[i];
		//data[i] = data[i + 2];
		//data[i + 2] = tmp;
		data_f[i] = data[i + 2];
		data_f[i + 1] = data[i + 1];
		data_f[i + 2] = data[i];
	}

	return data_f;
}



// from tutorial
// 因为是按字节分的所以修改了
#define u_opposite u_f[0]
#define u_ratio u_f[1]
#define v_opposite u_f[2]
#define v_ratio u_f[3]

void getBilinearFilteredPixelColor(float tex[], float u, float v, float res[])
{
	u *= map_width_i-1;
	v *= map_height_i-1;
	int x = u;
	int y = v;
	float u_f[4];
	u_ratio = u - x;
	v_ratio = v - y;
	u_opposite = 1 - u_ratio;
	v_opposite = 1 - v_ratio;
	x = max(0, x);
	y = max(0, y);
	//x = min(x, map_width_i-1);
	//y = min(y, map_height_i-1);
	int x_d, y_d;
	x_d = min(x + 1, map_width_i - 1);
	y_d = min(y + 1, map_height_i - 1);

	res[0] = (tex[(x + y * map_height_i) * 3] * u_opposite + tex[(x_d + y * map_height_i) * 3] * u_ratio) * v_opposite + (tex[(x + (y_d) * map_height_i) * 3] * u_opposite + tex[(x_d + (y_d) * map_height_i) * 3] * u_ratio) * v_ratio;
	res[1] = (tex[(x + y * map_height_i) * 3 + 1] * u_opposite + tex[(x_d + y * map_height_i) * 3 + 1] * u_ratio) * v_opposite + (tex[(x + (y_d) * map_height_i) * 3 + 1] * u_opposite + tex[(x_d + (y_d) * map_height_i) * 3 + 1] * u_ratio) * v_ratio;
	res[2] = (tex[(x + y * map_height_i) * 3 + 2] * u_opposite + tex[(x_d + y * map_height_i) * 3 + 2] * u_ratio) * v_opposite + (tex[(x + (y_d) * map_height_i) * 3 + 2] * u_opposite + tex[(x_d + (y_d) * map_height_i) * 3 + 2] * u_ratio) * v_ratio;
	//res[0] = tex[(x + y*map_height_i) * 3];
	//res[1] = tex[(x + y*map_height_i) * 3 + 1];
	//res[2] = tex[(x + y*map_height_i) * 3 + 2];
}

#define _trans(new_vec,i) 	new_vec[i] = t.right.vec[i] * vec_src[0]\
							+ t.up.vec[i] * vec_src[1]\
							+ t.forward.vec[i] * vec_src[2]\
							+ t.position.vec[i] * vec_src[3]

// 变换坐标

void transform(float* vec_src, float* vec_dst, Transform& t)
{
	float new_vec[4];
	for (int i = 0; i < 4; i++)
	{
		new_vec[i] = t.right.vec[i] * vec_src[0]
			+ t.up.vec[i] * vec_src[1]
			+ t.forward.vec[i] * vec_src[2]
			+ t.position.vec[i] * vec_src[3];
	}
	memcpy(vec_dst, new_vec, 4 * sizeof(float));
}

void transform_2(float* vec_src, float* vec_dst, Transform& t)
{
	float new_vec[4];

	new_vec[0] = vec_src[0] * t.right.vec[0]
		+ vec_src[1] * t.right.vec[1]
		+ vec_src[2] * t.right.vec[2]
		- (t.position * t.right);
	new_vec[1] = vec_src[0] * t.up.vec[0]
		+ vec_src[1] * t.up.vec[1]
		+ vec_src[2] * t.up.vec[2]
		- (t.position * t.up);
	new_vec[2] = vec_src[0] * t.forward.vec[0]
		+ vec_src[1] * t.forward.vec[1]
		+ vec_src[2] * t.forward.vec[2]
		- (t.position * t.forward);
	new_vec[3] = 1;

	memcpy(vec_dst, new_vec, 4 * sizeof(float));
}

//// 世界变换
//void WT(Reactangular& r0)
//{
//	for (int i = 0; i < 4; i++)
//	{
//		float* vec = r0.vertexs[i].vec;
//		transform_2(vec, r0.vertexs_world[i].vec, r0.transform);
//	}
//}

// 视角变换
void VT(Reactangular& r, Transform& t)
{
	for (int i = 0; i < 4; i++)
	{
		float* vec = r.vertexs_world[i].vec;
		float* dst = r.vertexs_view[i].vec;
		transform_2(vec, dst, t);
	}
}

// 透视投影
void get_Pt(Reactangular& r)
{
	//float tmp[4];
	for (int i = 0; i < 4; i++)
	{
		r.vertexs_sc[i].vec[0] = r.vertexs_view[i].vec[0] * cot_f * WINDOW_HEIGHT / WINDOW_WIDTH;
		r.vertexs_sc[i].vec[1] = r.vertexs_view[i].vec[1] * cot_f;
		r.vertexs_sc[i].vec[2] = r.vertexs_view[i].vec[2] * (z_far_f / (z_far_f - z_near_f)) - z_far_f * z_near_f / (z_far_f - z_near_f);
		r.vertexs_sc[i].vec[3] = r.vertexs_view[i].vec[2];

		r.vertexs_sc[i].vec[0] /= r.vertexs_sc[i].vec[3];
		r.vertexs_sc[i].vec[1] /= r.vertexs_sc[i].vec[3];
		r.vertexs_sc[i].vec[2] /= r.vertexs_sc[i].vec[3];

		//Pt[i].x = WINDOW_WIDTH*(r0.vertexs_view[i].vec[0] * (3/4) / r0.vertexs_view[i].vec[2] + 1) / 2;
		//Pt[i].y = WINDOW_HEIGHT*(r0.vertexs_view[i].vec[1] / r0.vertexs_view[i].vec[2] + 1 ) /2 ;
	}
	// 叉积求法向
	Vector4 t0 = r.vertexs_sc[1] - r.vertexs_sc[0];
	Vector4 t1 = r.vertexs_sc[2] - r.vertexs_sc[0];
	r.transform.forward = (t1 / t0).normal();
}

// RGB 运算
inline int do_RGB(float Rd, float Gd, float Bd)
{
	int R = min(255, Rd);
	int G = min(255, Gd);
	int B = min(255, Bd);
	return R << 16 | G << 8 | B;
}

// 简化幂运算
inline float my_pow(float a, int n)
{
	for (size_t i = 0; i < n; i++)
	{
		a = a * a;
	}
	return a;
}

inline double f(const int a, const int b, const int x, const int y, const mPOINT p[])
{
	return (p[a].y - p[b].y) * (x - p[b].x) + (p[b].x - p[a].x) * (y - p[b].y);
}

//#define f(a,b,xx,yy,p) (float)(((p[a].y - p[b].y) * (xx - p[b].x) + (p[b].x - p[a].x) * (yy - p[b].y)))

bool fast_judge(const double* const a)
{
	//byte *t = (byte*)a;
	//if ((t[7] & 0x80) == 0x80) return false;
	//int exp = (t[7] & 0x7F) << 4 | ((t[6] >> 4) & 0x0F);
	//if (exp > 1023) return false;
	//return true;
	return (*a > -1e-3);
}

float get_light(double ss[], Vector4 vecs[], Vector4& light_spot, Vector4& forward, Vector4& view_pos, int type)
{
	Vector4 point;
	// 光照
	Vector4 light;
	// 漫反射
	Vector4 reflect;
	Vector4 view_dir;
	float diff;
	float spec;
	float zr;
	float u;
	float v;
	float color[3];


	// 点的坐标
	if (type == 0)
		point = ss[0] * vecs[0] + ss[1] * vecs[1] + ss[2] * vecs[2];
	else
		point = ss[0] * vecs[2] + ss[1] * vecs[3] + ss[2] * vecs[0];

	// 光照
	light = point - light_spot;
	// 漫反射
	diff = light * forward * light_mod;
	if (diff < 0.1)
		return -1;
	//diff = max(0, diff);
	// 全局光
	//diff += 0.1;
	// 反射
	// mod()为取长度倒数
	reflect = 2.0 * diff * (forward)-light_mod * light;
	//__m128 reflect_m = Vector4::sub(2.0 * diff * (forward), light_mod * light);
	//if (reflect_mod == 0)
	//reflect_mod = reflect.mod();
	// 视角方向
	//view_dir = point - view_pos;
	__m128 viewdir_m = Vector4::sub(point, view_pos);
	spec = 0;
	//if (diff > 0.4)
	////if (diff < 0.2) return -1;
	//float tmp2 = max(0, Vector4::mul(reflect_m, viewdir_m));
	//float tmp = tmp2 * (Vector4::mod(reflect_m)* Vector4::mod(viewdir_m));
	float tmp2 = max(0, reflect * view_dir);
	float tmp = tmp2 * reflect.mod() * view_dir.mod();
	spec = my_pow(tmp, specular_i);
	//min(spec, 1e10);
	// 透视矫正
	if (type == 0)
	{
		zr = ss[0] / vecs[0].vec[2] + ss[1] / vecs[1].vec[2] + ss[2] / vecs[2].vec[2];
		u = ((ss[0] * (0 / vecs[2].vec[2]) + ss[1] * (0 / vecs[1].vec[2]) + ss[2] * (1 / vecs[2].vec[2])) / zr);
		v = ((ss[0] * (0 / vecs[2].vec[2]) + ss[1] * (1 / vecs[1].vec[2]) + ss[2] * (1 / vecs[2].vec[2])) / zr);
	}
	else
	{
		zr = ss[0] / vecs[2].vec[2] + ss[1] / vecs[3].vec[2] + ss[2] / vecs[0].vec[2];
		u = ((ss[0] * (1 / vecs[2].vec[2]) + ss[1] * (1 / vecs[3].vec[2]) + ss[2] * (0 / vecs[0].vec[2])) / zr);
		v = ((ss[0] * (1 / vecs[2].vec[2]) + ss[1] * (0 / vecs[3].vec[2]) + ss[2] * (0 / vecs[0].vec[2])) / zr);
	}
	//float u = ss[0];
	//float v = ss[2];
	// 纹理过滤
	getBilinearFilteredPixelColor(map, u, v, color);
	return do_RGB((diff) * (color[0]) + ((spec) * (0xFF)),
	              (diff) * (color[1]) + ((spec) * (0xFF)),
	              (diff) * (color[2]) + ((spec) * (0xFF)));
}

// 光栅化
// 由于直接传整个矩形进来，因此分割成两个三角形做
float check_point(int x, int y, mPOINT p[], Vector4* vecs, Vector4& light_spot, Vector4& forward, Vector4& view_pos,
				int f12, int f20, int f30, int f02, int f12_a, int f20_b, int f30_a, int f02_b)
{
	// 颜色
	float color[3];
	// 重心坐标
	//int x0 = p[0].x;
	//int y0 = p[0].y;
	//float t1 = f(1, 2, x, y, p);
	//float t2 = f(1, 2, x0, y0, p);
	//float a = f(1, 2, x, y, p) / f(1, 2, p[0].x, p[0].y, p);
	double ss[3];
	//ss[0] = f(1, 2, x, y, p) / f12;
	//ss[1] = f(2, 0, x, y, p) / f20;
	ss[0] = 1.0*f12_a / f12;
	ss[1] = 1.0*f20_b / f20;
	ss[2] = 1 - ss[0] - ss[1];
	// 点的坐标
	Vector4 point;
	// 光照
	Vector4 light;
	// 漫反射
	Vector4 reflect;
	Vector4 view_dir;
	float diff;
	float spec;
	float zr;
	float u;
	float v;
	//float c = 1 - a - b;
	// 判断是否在内部
	if (fast_judge(ss) && fast_judge(&ss[1]) && fast_judge(&ss[2]))
	{
		return get_light(ss, vecs, light_spot, forward, view_pos, 0);
		//// 点的坐标
		//point = ss[0] * vecs[0] + ss[1] * vecs[1] + ss[2] * vecs[2];
		//// 光照
		//light = point - light_spot;
		//// 漫反射
		//diff = light * forward * light_mod;
		//diff = max(0, diff);
		//// 全局光
		////diff += 0.1;
		//// 反射
		//// mod()为取长度倒数
		//reflect = 2.0 * diff * (forward) - light_mod * light;
		////if (reflect_mod == 0)
		//	//reflect_mod = reflect.mod();
		//// 视角方向
		//view_dir = point - view_pos;
		//spec = 0;
		////if (diff > 0.4)
		//	spec = my_pow(max(0, reflect*view_dir*view_dir.mod()*reflect.mod()), specular_i);
		////min(spec, 1e10);
		//// 透视矫正
		//zr = ss[0] / vecs[0].vec[2] + ss[1] / vecs[1].vec[2] + ss[2]/ vecs[2].vec[2];
		//u = ((ss[0] * (0 / vecs[2].vec[2]) + ss[1] * (0 / vecs[1].vec[2]) + ss[2]*(1 / vecs[2].vec[2])) / zr);
		//v = ((ss[0] * (0 / vecs[2].vec[2]) + ss[1] * (1 / vecs[1].vec[2]) + ss[2]*(1 / vecs[2].vec[2])) / zr);
		////float u = ss[0];
		////float v = ss[2];
		//// 纹理过滤
		//getBilinearFilteredPixelColor(map, u, v,color);
		//return do_RGB((diff)* (color[0]) + ((spec)* (0xFF)),
		//	(diff)* (color[1]) + ((spec)* (0xFF)),
		//	(diff)* (color[2]) + ((spec)* (0xFF)));
	}
	// 对另外一个三角形做同样处理
	else if (ss[1] < 0)
	{
		//ss[0] = f(3, 0, x, y, p) / f30;
		//ss[1] = f(0, 2, x, y, p) / f02;
		ss[0] = 1.0*f30_a / f30;
		ss[1] = 1.0*f02_b / f02;
		ss[2] = 1 - ss[0] - ss[1];
		if (fast_judge(ss) && fast_judge(&ss[1]) && fast_judge(&ss[2]))
		{
			//point = ss[0] * vecs[2] + ss[1] * vecs[3] + ss[2]*vecs[0];
			//light = point - light_spot;
			//diff = light * forward * light_mod;
			//diff = max(0, diff);
			////diff += 0.1;
			//reflect = 2.0 * diff * (forward) - light_mod * light;
			////if (reflect_mod == 0)
			//	//reflect_mod = reflect.mod();
			//view_dir = point - view_pos;
			//spec = 0;
			////if (diff > 0.4)
			//	spec = my_pow(max(0, reflect*view_dir*view_dir.mod()*reflect.mod()), specular_i);

			//zr = ss[0] / vecs[2].vec[2] + ss[1] / vecs[3].vec[2] + ss[2]/ vecs[0].vec[2];
			//u = ((ss[0] * (1 / vecs[2].vec[2]) + ss[1] * (1 / vecs[3].vec[2]) + ss[2]*(0 / vecs[0].vec[2])) / zr);
			//v = ((ss[0] * (1 / vecs[2].vec[2]) + ss[1] * (0 / vecs[3].vec[2]) + ss[2]*(0 / vecs[0].vec[2])) / zr);

			//getBilinearFilteredPixelColor(map, u, v, color);
			//return do_RGB((diff)* (color[0]) + ((spec)* (0xFF)),
			//	(diff)* (color[1]) + ((spec)* (0xFF)),
			//	(diff)* (color[2]) + ((spec)* (0xFF)));
			return get_light(ss, vecs, light_spot, forward, view_pos, 1);
		}
	}
	// 在外部
	return -100;
}

void init(HWND hWnd, HINSTANCE hInstance)
{
	map = readBMP(map_name_str);
	hDC = GetDC(hWnd);
	Memhdc = CreateCompatibleDC(hDC);
	Membitmap = CreateCompatibleBitmap(hDC, WINDOW_WIDTH, WINDOW_HEIGHT);
	SelectObject(Memhdc, Membitmap);
	BITMAPINFO bmp_info;
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

	buffer = (unsigned int *)malloc(sizeof(int) * WINDOW_HEIGHT * WINDOW_WIDTH);

	now_bitmap = CreateDIBSection(Memhdc, &bmp_info, DIB_RGB_COLORS, (void**)&buffer, NULL, 0);

	// 精确计时
	QueryPerformanceFrequency(&tf);
	QueryPerformanceCounter(&t0);

	// 初始化立方体
	//reacs[0].transform.forward = Vector4(0, 0, -1, 0);
	//reacs[0].transform.up = Vector4(0, 1, 0, 0);
	//reacs[0].transform.right = Vector4(-1, 0, 0, 0);
	Vector4 points[8];
	points[0] = Vector4(-SIZE / 2, -SIZE / 2, -SIZE / 2, 1);
	points[1] = Vector4(SIZE / 2, -SIZE / 2, -SIZE / 2, 1);
	points[2] = Vector4(SIZE / 2, SIZE / 2, -SIZE / 2, 1);
	points[3] = Vector4(-SIZE / 2, SIZE / 2, -SIZE / 2, 1);
	points[4] = Vector4(-SIZE / 2, -SIZE / 2, SIZE / 2, 1);
	points[5] = Vector4(SIZE / 2, -SIZE / 2, SIZE / 2, 1);
	points[6] = Vector4(SIZE / 2, SIZE / 2, SIZE / 2, 1);
	points[7] = Vector4(-SIZE / 2, SIZE / 2, SIZE / 2, 1);
	//reacs[1].transform.forward = Vector4(0, 1, 0, 0);
	//reacs[1].transform.up = Vector4(0, 0, 1, 0);
	//reacs[1].transform.right = Vector4(-1, 0, 0, 0);
	int table[6][4] = {
		{ 0, 3, 2, 1 }, 
		{ 1, 2, 6, 5 }, 
		{ 5, 6, 7, 4 }, 
		{ 4, 7, 3, 0 }, 
		{ 2, 3, 7, 6 }, 
		{ 0, 1, 5, 4 }
	};
	for (size_t i = 0; i < 6; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			reacs[i].vertexs_world[j] = points[table[i][j]];
		}
	}
	camera.forward = Vector4(0, 0, -1, 0);
	camera.up = Vector4(0, 1, 0, 0);
	camera.right = Vector4(-1, 0, 0, 0);
	camera.position = Vector4(0, 0, 50, 1);
}

void rotate_x(float* vec, float c, float s)
{
	float new_vec[3];
	new_vec[0] = vec[0];
	new_vec[1] = vec[1] * c + vec[2] * s;
	new_vec[2] = vec[1] * -s + vec[2] * c;
	memcpy(vec, new_vec, 3 * sizeof(float));
}

void rotate_y(float* vec, float c, float s)
{
	float new_vec[3];
	new_vec[0] = vec[0] * c + vec[2] * s;
	new_vec[1] = vec[1];
	new_vec[2] = vec[0] * -s + vec[2] * c;
	memcpy(vec, new_vec, 3 * sizeof(float));
}

void rotate_z(float* vec, float c, float s)
{
	float new_vec[3];
	new_vec[0] = vec[0] * c + vec[1] * s;
	new_vec[1] = vec[0] * -s + vec[1] * c;
	new_vec[2] = vec[2];
	memcpy(vec, new_vec, 3 * sizeof(float));
}

void GameLoop(HWND hwnd)
{
	static mPOINT Pt[4 * 6];
	static Zbuffer buffers[6];
	static int count = 0;

	// 开始计时
	QueryPerformanceCounter(&t1);

	Vector4 light(0, 0, -40, 0);
	// 光照跟随摄像头
	// 取消下面注释，光线在世界坐标
	//transform_2(light.vec, light.vec, camera); 

	// 对每个面
#pragma omp parallel for
	for (int i = 0; i < 6; i++)
	{
		// 视角坐标
		VT(reacs[i], camera);
		// 顶点投影
		get_Pt(reacs[i]);
		buffers[i].i = i;
		if (reacs[i].transform.forward.vec[2] > 0)
		{
			buffers[i].z = reacs[i].get_z();
		}
		else
		{
			buffers[i].z = -1;
		}
	}


	// zsort 对矩形根据深度排序
	vector<Zbuffer> bsort(6);
	bsort.assign(buffers, buffers + 6);

	//sort(bsort.begin(), bsort.end(), [](const Zbuffer& x, const Zbuffer& y)
	//     {
	//	     return x.z > y.z;
	//     });
	light_mod = light.mod();
	for (Zbuffer z : bsort)
	{
		if (z.z < 0)
			continue;
		//if (z.i != 1)
		//	continue;
		mPOINT p[4];
		for (size_t i = 0; i < 4; i++)
		{
			p[i].x = (reacs[z.i].vertexs_sc[i].vec[0] + 1) / 2 * WINDOW_WIDTH;
			p[i].y = (reacs[z.i].vertexs_sc[i].vec[1] + 1) / 2 * WINDOW_HEIGHT;
		}
		int minx, miny, maxx, maxy;
		float minx_p = WINDOW_WIDTH, miny_p = WINDOW_HEIGHT, maxx_p = 0, maxy_p = 0;
		//bool t0 = true;
		// 计算包裹了投影四边形的矩形

		for (int i = 0; i < 4; ++i)
		{
			mPOINT* pp = p + i;
			if (minx_p > pp->x) minx_p = pp->x;
			if (miny_p > pp->y) miny_p = pp->y;
			if (maxx_p < pp->x) maxx_p = pp->x;
			if (maxy_p < pp->y) maxy_p = pp->y;
			//if (pp->x > 0 && pp->x < WINDOW_WIDTH && pp->y > 0 && pp->y < WINDOW_HEIGHT) t0 = false;
		}
		float midx = (minx_p + maxx_p) / 2;
		float midy = (miny_p + maxy_p) / 2;
		maxx = min(WINDOW_WIDTH-1, maxx_p+2);
		maxy = min(WINDOW_HEIGHT-1, maxy_p+2);
		minx = max(0, minx_p-2);
		miny = max(0, miny_p-2);
		// 扫描
		int i;
		char has_printed[WINDOW_HEIGHT];
		char skip[WINDOW_HEIGHT];
		memset(has_printed, 0, WINDOW_HEIGHT * sizeof(char));
		memset(skip, 0, WINDOW_HEIGHT * sizeof(char));
		float f12 = f(1, 2, p[0].x, p[0].y, p);
		float f20 = f(2, 0, p[1].x, p[1].y, p);
		float f30 = f(3, 0, p[2].x, p[2].y, p);
		float f02 = f(0, 2, p[3].x, p[3].y, p);

#pragma omp parallel for schedule(dynamic, 100)
		for (i = miny; i < maxy; i++)
		{
			int f12_a = f(1, 2, minx, i, p);
			int f20_b = f(2, 0, minx, i, p);
			int f30_a = f(3, 0, minx, i, p);
			int f02_b = f(0, 2, minx, i, p);
			int j;
			//if (reacs[z.i].transform.forward.vec[0] > 0)
			{
				for (j = minx; j < maxx; j++)
				{
					if (*(buffer + i * WINDOW_WIDTH + j) > 0x0)
						continue;
					// 如果已经画了点就不扫描了
					int color = check_point(j, i, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward, camera.forward, 
						f12,
						f20,
						f30,
						f02,
						f12_a + (p[1].y - p[2].y) * (j - minx),
						f20_b + (p[2].y - p[0].y) * (j - minx),
						f30_a + (p[3].y - p[0].y) * (j - minx),
						f02_b + (p[0].y - p[2].y) * (j - minx)
						);
					if (color >= 0)
					{
						*(buffer + i * WINDOW_WIDTH + j) = color;
					}
				}
			}
			//else
			//{
			//	for (j = maxx; j > minx; j--)
			//	{
			//		//if (*(buffer + i * WINDOW_WIDTH + j) > 0x0)
			//		//	continue;
			//		//if (skip[i] > 0)
			//		//{
			//		//	continue;
			//		//}
			//		// 如果已经画了点就不扫描了
			//		int color = check_point(j, i, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward, camera.forward,
			//			f12,
			//			f20,
			//			f30,
			//			f02);
			//		if (color >= 0)
			//		{
			//			*(buffer + i * WINDOW_WIDTH + j) = color;
			//			has_printed[i] = 1;;
			//		}
			//		//else
			//		//{
			//		//	if (has_printed[i] > 0)
			//		//		skip[i] = 1;
			//		//}
			//	}
			//}
		}
	}

	count += 1;

	static char tmp[100] = "FPS:\0";
	QueryPerformanceCounter(&t2);

	int frames = tf.QuadPart / (t2.QuadPart - t1.QuadPart);
	if (count == 10)
	{
		QueryPerformanceFrequency(&tf);

		sprintf(tmp, "FPS: %d\0", frames);
		count = 0;
	}
	// 根据当前帧数判断旋转幅度
	//frames = 60;
	camera.forward = Vector4(0, 0, -1, 0);
	camera.up = Vector4(0, 1, 0, 0);
	camera.right = Vector4(-1, 0, 0, 0);
	camera.position = Vector4(0, 0, 50, 1);
	static float r_x = 0, r_y = 0;

	r_x += 1.0*a_x * 360 / frames;
	r_y += 1.0*a_y * 360 / frames;
	r_x = min(90, r_x);
	r_x = max(-90, r_x);
	//float cx = cos(3.14 * (60.0 / frames) * a_x / 20);
	//float sx = sin(3.14 * (60.0 / frames) * a_x / 20);
	//float cy = cos(3.14 * (60.0 / frames) * a_y / 20);
	//float sy = sin(3.14 * (60.0 / frames) * a_y / 20);
	//float cz = cos(3.14 * (60.0 / frames) * a_z / 20);
	//float sz = sin(3.14 * (60.0 / frames) * a_z / 20);
	float cx = cos(r_x / 180 * 3.14);
	float sx = sin(r_x / 180 * 3.14);
	float cy = cos(r_y / 180 * 3.14);
	float sy = sin(r_y / 180 * 3.14);
	float cz = cos(3.14 * (60.0 / frames) * a_z / 20);
	float sz = sin(3.14 * (60.0 / frames) * a_z / 20);
	rotate_x(camera.forward.vec, cx, sx);
	rotate_x(camera.up.vec, cx, sx);
	rotate_x(camera.right.vec, cx, sx);

	rotate_y(camera.right.vec, cy, sy);
	rotate_y(camera.forward.vec, cy, sy);
	rotate_y(camera.up.vec, cy, sy);

	rotate_z(camera.forward.vec, cz, sz);
	rotate_z(camera.up.vec, cz, sz);
	rotate_z(camera.right.vec, cz, sz);
	//if (a_x != 0)
	rotate_x(camera.position.vec, cx, sx);
	rotate_y(camera.position.vec, cy, sy);
	//if (a_y != 0)
	//rotate_z(camera.position.vec, cz, sz);
	static float distance = 1;
	distance += a_dep / 60.0;
	camera.position = distance * camera.position;
	a_dep = 0;
	a_x = 0;
	a_y = 0;
	a_z = 0;
	//camera.forward = -camera.position.normal();
	//camera.right = -(camera.forward / Vector4(0, 1, 0, 0)).normal();
	//camera.up = (camera.forward / camera.right);

	//QueryPerformanceCounter(&t0);
	char _x[50] = "X:";
	char _y[50] = "Y:";
	char _z[50] = "Z:";
	sprintf(_x, "X:%d", a_x);
	sprintf(_y, "Y:%d", a_y);
	sprintf(_z, "Z:%d", a_z);
	// 设置文字背景色
	SetBkColor(Memhdc, RGB(0, 0, 0));
	// 设置文字颜色
	SetTextColor(Memhdc, RGB(255, 255, 255));
	TextOut(Memhdc, 20, 450, tmp, strlen(tmp));
	TextOut(Memhdc, 20, 470, _x, strlen(_x));
	TextOut(Memhdc, 20, 490, _y, strlen(_y));
	TextOut(Memhdc, 20, 510, _z, strlen(_z));
	// float buffer
	SelectObject(Memhdc, now_bitmap);
	BitBlt(hDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Memhdc, 0, 0, SRCCOPY);


	memset(buffer, 0x00, sizeof(int) * WINDOW_WIDTH * WINDOW_HEIGHT);
}

LRESULT CALLBACK WindProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_COMMAND:
		break;
		// 按键处理
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			a_y = -1;
			break;
		case VK_RIGHT:
			a_y = 1;
			break;
		case VK_UP:
			a_x = 1;
			break;
		case VK_DOWN:
			a_x = -1;
			break;
		case VK_F1:
			a_dep++;
			break;
		case VK_F2:
			a_dep--;
			break;
		case VK_HOME:
			a_z++;
			break;
		case VK_END:
			a_z--;
			break;
		}
		// 重新计算
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
	omp_set_num_threads(thread_count_b);

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
