#define _CRT_SECURE_NO_WARNINGS
#define EIGEN_NO_DEBUG 
#include <windows.h>
#include <string>   
#include <vector>
#include <algorithm>

using namespace std;

#include "Cube.h"
#include "Zbuffer.h"
#include <math.h>
#include <omp.h>
#include <Eigen/Dense>
#include <Eigen/Core>

//����
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

//int macro_size = 16;
//byte thread_count_b = 4; // openmp thread count

float z_far_f = 1000;
float z_near_f = 0.1;
char map_name_str[] = "map1.bmp"; // bitmap
static int specular_i = 6; // specular 2^6=64
static float delta_f = 1e-5; // delta
int map_width_i = -1; //	size of bitmap
int map_height_i = -1;

// �ӽ�
const float cot_f = 3;
// ������̬
int a_dep = 0;
int a_x = 0;
int a_y = 0;
int a_z = 0;

// hack
float light_mod = 0;
//float reflect_mod = 0;
// float buffer
unsigned int* buffer;

float* __map_data;
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

//----------��������---------------
void init(HWND hWnd, HINSTANCE hInstance);


//-----�ص�����-----------------
LRESULT CALLBACK WindProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// read bitmap
// û�д����������ͼƬ�ֱ�����Ҫ4��������
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
// ��Ϊ�ǰ��ֽڷֵ������޸���
#define u_opposite u_f[0]
#define u_ratio u_f[1]
#define v_opposite u_f[2]
#define v_ratio u_f[3]

void getBilinearFilteredPixelColor(float* tex, float u, float v, float res[])
{
	u *= map_width_i - 1;
	v *= map_height_i - 1;
	int x = u;
	int y = v;
	float u_f[4];
	u_ratio = u - x;
	v_ratio = v - y;
	u_opposite = 1 - u_ratio;
	v_opposite = 1 - v_ratio;
	x = max(0, x);
	y = max(0, y);
	x = min(x, map_width_i-1);
	y = min(y, map_height_i-1);
	int x_d, y_d;
	x_d = min(x + 1, map_width_i - 1);
	y_d = min(y + 1, map_height_i - 1);

	res[0] = (tex[(x + y * map_height_i) * 3] * u_opposite + tex[(x_d + y * map_height_i) * 3] * u_ratio) * v_opposite + (tex[(x + (y_d) * map_height_i) * 3] * u_opposite + tex[(x_d + (y_d) * map_height_i) * 3] * u_ratio) * v_ratio;
	res[1] = (tex[(x + y * map_height_i) * 3 + 1] * u_opposite + tex[(x_d + y * map_height_i) * 3 + 1] * u_ratio) * v_opposite + (tex[(x + (y_d) * map_height_i) * 3 + 1] * u_opposite + tex[(x_d + (y_d) * map_height_i) * 3 + 1] * u_ratio) * v_ratio;
	res[2] = (tex[(x + y * map_height_i) * 3 + 2] * u_opposite + tex[(x_d + y * map_height_i) * 3 + 2] * u_ratio) * v_opposite + (tex[(x + (y_d) * map_height_i) * 3 + 2] * u_opposite + tex[(x_d + (y_d) * map_height_i) * 3 + 2] * u_ratio) * v_ratio;
}

// �任����

void transform_2(float* vec_src, float* vec_dst, Transform& t)
{
	float new_vec[3];

	new_vec[0] = vec_src[0] * t.right.data()[0]
		+ vec_src[1] * t.right.data()[1]
		+ vec_src[2] * t.right.data()[2]
		- (t.position.dot(t.right));
	new_vec[1] = vec_src[0] * t.up.data()[0]
		+ vec_src[1] * t.up.data()[1]
		+ vec_src[2] * t.up.data()[2]
		- (t.position.dot(t.up));
	new_vec[2] = vec_src[0] * t.forward.data()[0]
		+ vec_src[1] * t.forward.data()[1]
		+ vec_src[2] * t.forward.data()[2]
		- (t.position.dot(t.forward));

	memcpy(vec_dst, new_vec, 3 * sizeof(float));
}

// �ӽǱ任
void VT(Reactangular& r, Transform& t)
{
	for (int i = 0; i < 4; i++)
	{
		float* vec = r.vertexs_world[i].data();
		float* dst = r.vertexs_view[i].data();
		transform_2(vec, dst, t);
	}
}

// ͸��ͶӰ
void get_Pt(Reactangular& r)
{
	//float tmp[4];
	for (int i = 0; i < 4; i++)
	{
		r.vertexs_sc[i].data()[0] = r.vertexs_view[i].data()[0] * cot_f * WINDOW_HEIGHT / WINDOW_WIDTH;
		r.vertexs_sc[i].data()[1] = r.vertexs_view[i].data()[1] * cot_f;
		r.vertexs_sc[i].data()[2] = r.vertexs_view[i].data()[2] * (z_far_f / (z_far_f - z_near_f)) - z_far_f * z_near_f / (z_far_f - z_near_f);
		r.vertexs_sc[i].data()[3] = r.vertexs_view[i].data()[2];
		r.vertexs_sc[i].data()[0] /= r.vertexs_sc[i].data()[3];
		r.vertexs_sc[i].data()[1] /= r.vertexs_sc[i].data()[3];
		r.vertexs_sc[i].data()[2] /= r.vertexs_sc[i].data()[3];
	}
	// �������
	Eigen::Vector3f t0 = r.vertexs_sc[1] - r.vertexs_sc[0];
	Eigen::Vector3f t1 = r.vertexs_sc[2] - r.vertexs_sc[0];
	//r.transform.forward = (t1 * t0).normalized();
	Vector4::mul(t1.data(), t0.data(), r.transform.forward.data());
	r.transform.forward = r.transform.forward.normalized();
}

// RGB ����
inline int do_RGB(float Rd, float Gd, float Bd)
{
	int R = min(255, Rd);
	int G = min(255, Gd);
	int B = min(255, Bd);
	return R << 16 | G << 8 | B;
}

// ��������
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

bool fast_judge(const float* const a)
{
	//byte *t = (byte*)a;
	//if ((t[7] & 0x80) == 0x80) return false;
	//int exp = (t[7] & 0x7F) << 4 | ((t[6] >> 4) & 0x0F);
	//if (exp > 1023) return false;
	//return true;
	return (*a > -1e-3);
}

float get_light(float ss[], Eigen::Vector3f vecs[], Eigen::Vector3f& light_spot, Eigen::Vector3f& forward, Eigen::Vector3f& view_pos, int type)
{
	Eigen::Vector3f point;
	// ����
	Eigen::Vector3f light;
	// ������
	Eigen::Vector3f reflect;
	Eigen::Vector3f view_dir;
	float diff;
	float spec;
	float zr;
	float u;
	float v;
	float color[3];


	// �������
	if (type == 0)
		point = ss[0] * vecs[0] + ss[1] * vecs[1] + ss[2] * vecs[2];
	// ���� amp 
	//Vector4::vecAdd(ss, vecs[0].data(), vecs[1].data(), vecs[2].data(),point.data(),3);
	else
		point = ss[0] * vecs[2] + ss[1] * vecs[3] + ss[2] * vecs[0];
	//Vector4::vecAdd(ss, vecs[2].data(), vecs[3].data(), vecs[0].data(), point.data(), 3);


	// ����
	light = point - light_spot;
	// ������
	diff = light.normalized().dot(forward);
	// ����
	//reflect = 2.0 * diff * (forward)-light_mod*light;
	reflect = 2.0 * diff * (forward)-light.normalized();
	// �ӽǷ���
	view_dir = point - view_pos;
	float tmp2 = reflect.normalized().dot(view_dir.normalized());

	spec = my_pow(tmp2, specular_i);
	// ͸�ӽ���
	if (type == 0)
	{
		zr = ss[0] / vecs[0].data()[2] + ss[1] / vecs[1].data()[2] + ss[2] / vecs[2].data()[2];
		u = ((ss[0] * (0 / vecs[2].data()[2]) + ss[1] * (0 / vecs[1].data()[2]) + ss[2] * (1 / vecs[2].data()[2])) / zr);
		v = ((ss[0] * (0 / vecs[2].data()[2]) + ss[1] * (1 / vecs[1].data()[2]) + ss[2] * (1 / vecs[2].data()[2])) / zr);
	}
	else
	{
		zr = ss[0] / vecs[2].data()[2] + ss[1] / vecs[3].data()[2] + ss[2] / vecs[0].data()[2];
		u = ((ss[0] * (1 / vecs[2].data()[2]) + ss[1] * (1 / vecs[3].data()[2]) + ss[2] * (0 / vecs[0].data()[2])) / zr);
		v = ((ss[0] * (1 / vecs[2].data()[2]) + ss[1] * (0 / vecs[3].data()[2]) + ss[2] * (0 / vecs[0].data()[2])) / zr);
	}
	//float u = ss[0];
	//float v = ss[2];
	// �������
	getBilinearFilteredPixelColor(__map_data, u, v, color);
	return do_RGB((diff) * (color[0]) + ((spec) * (0xFF)),
	              (diff) * (color[1]) + ((spec) * (0xFF)),
	              (diff) * (color[2]) + ((spec) * (0xFF)));
}

// ��դ��
// ����ֱ�Ӵ��������ν�������˷ָ��������������
float check_point(int x, int y, mPOINT p[], Eigen::Vector3f* vecs, Eigen::Vector3f& light_spot, Eigen::Vector3f& forward, Eigen::Vector3f& view_pos,
                  float f0a, float f0b, float f1a, float f1b)
{
	// ��������

	float ss[3];

	ss[0] = f0a;
	ss[1] = f0b;
	ss[2] = 1 - ss[0] - ss[1];
	// �ж��Ƿ����ڲ�
	if (fast_judge(ss) && fast_judge(&ss[1]) && fast_judge(&ss[2]))
	{
		return get_light(ss, vecs, light_spot, forward, view_pos, 0);
	}
	// ������һ����������ͬ������
	else if (ss[1] < 0)
	{
		ss[0] = f1a;
		ss[1] = f1b;
		ss[2] = 1 - ss[0] - ss[1];
		if (fast_judge(ss) && fast_judge(&ss[1]) && fast_judge(&ss[2]))
		{
			return get_light(ss, vecs, light_spot, forward, view_pos, 1);
		}
	}
	// ���ⲿ
	return -100;
}

void init(HWND hWnd, HINSTANCE hInstance)
{
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
	Eigen::Vector3f points[8];
	points[0] = Eigen::Vector3f(-SIZE / 2, -SIZE / 2, -SIZE / 2);
	points[1] = Eigen::Vector3f(SIZE / 2, -SIZE / 2, -SIZE / 2);
	points[2] = Eigen::Vector3f(SIZE / 2, SIZE / 2, -SIZE / 2);
	points[3] = Eigen::Vector3f(-SIZE / 2, SIZE / 2, -SIZE / 2);
	points[4] = Eigen::Vector3f(-SIZE / 2, -SIZE / 2, SIZE / 2);
	points[5] = Eigen::Vector3f(SIZE / 2, -SIZE / 2, SIZE / 2);
	points[6] = Eigen::Vector3f(SIZE / 2, SIZE / 2, SIZE / 2);
	points[7] = Eigen::Vector3f(-SIZE / 2, SIZE / 2, SIZE / 2);
	int table[6][4] = {
		{0, 3, 2, 1},
		{1, 2, 6, 5},
		{5, 6, 7, 4},
		{4, 7, 3, 0},
		{2, 3, 7, 6},
		{0, 1, 5, 4}
	};
	for (size_t i = 0; i < 6; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			reacs[i].vertexs_world[j] = points[table[i][j]];
		}
	}
	camera.forward = Eigen::Vector3f(0, 0, -1);
	camera.up = Eigen::Vector3f(0, 1, 0);
	camera.right = Eigen::Vector3f(-1, 0, 0);
	camera.position = Eigen::Vector3f(0, 0, 50);
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
	//static mPOINT Pt[4 * 6];
	static Zbuffer buffers[6];
	static int count = 0;

	// ��ʼ��ʱ
	QueryPerformanceCounter(&t1);

	Eigen::Vector3f light(0, 0, -40);
	// ���ո�������ͷ
	// ȡ������ע�ͣ���������������
	//transform_2(light.vec, light.vec, camera); 

	// ��ÿ����
	//#pragma omp parallel for
	for (int i = 0; i < 6; i++)
	{
		// �ӽ�����
		VT(reacs[i], camera);
		// ����ͶӰ
		get_Pt(reacs[i]);
		buffers[i].i = i;
		if (reacs[i].transform.forward.data()[2] > 0)
		{
			buffers[i].z = 1;
		}
		else
		{
			buffers[i].z = -1;
		}
	}


	// zsort �Ծ��θ����������
	vector<Zbuffer> bsort(6);
	bsort.assign(buffers, buffers + 6);
	for (Zbuffer z : bsort)
	{
		if (z.z < 0)
			continue;
		mPOINT p[4];
		for (size_t i = 0; i < 4; i++)
		{
			p[i].x = (reacs[z.i].vertexs_sc[i].data()[0] + 1) / 2 * WINDOW_WIDTH;
			p[i].y = (reacs[z.i].vertexs_sc[i].data()[1] + 1) / 2 * WINDOW_HEIGHT;
		}
		int minx, miny, maxx, maxy;
		float minx_p = WINDOW_WIDTH, miny_p = WINDOW_HEIGHT, maxx_p = 0, maxy_p = 0;
		// ���������ͶӰ�ı��εľ���

		for (int i = 0; i < 4; ++i)
		{
			mPOINT* pp = p + i;
			if (minx_p > pp->x) minx_p = pp->x;
			if (miny_p > pp->y) miny_p = pp->y;
			if (maxx_p < pp->x) maxx_p = pp->x;
			if (maxy_p < pp->y) maxy_p = pp->y;
		}
		maxx = min(WINDOW_WIDTH-1, maxx_p+2);
		maxy = min(WINDOW_HEIGHT-1, maxy_p+2);
		minx = max(0, minx_p-2);
		miny = max(0, miny_p-2);
		// ɨ��
		int i;
		char has_printed[WINDOW_HEIGHT];
		char skip[WINDOW_HEIGHT];
		memset(has_printed, 0, WINDOW_HEIGHT * sizeof(char));
		memset(skip, 0, WINDOW_HEIGHT * sizeof(char));
		float f12 = f(1, 2, p[0].x, p[0].y, p);
		float f20 = f(2, 0, p[1].x, p[1].y, p);
		float f30 = f(3, 0, p[2].x, p[2].y, p);
		float f02 = f(0, 2, p[3].x, p[3].y, p);
#pragma omp parallel for schedule(dynamic,4)
//#pragma omp parallel for

		for (i = miny; i < maxy; i++)
		{
			int f12_a = f(1, 2, minx, i, p);
			int f20_b = f(2, 0, minx, i, p);
			int f30_a = f(3, 0, minx, i, p);
			int f02_b = f(0, 2, minx, i, p);

			float f0a = 1.0 * f12_a / f12;
			float f0b = 1.0 * f20_b / f20;
			float f1a = 1.0 * f30_a / f30;
			float f1b = 1.0 * f02_b / f02;
			int j;
			//if (reacs[z.i].transform.forward.data()[0] > 0.0001)
			{
				for (j = minx; j < maxx; j++)
				{
					if (*(buffer + i * WINDOW_WIDTH + j) > 0x0)
					{
						if (has_printed[i])
							skip[i] = -1;
						continue;
					}
					//if (skip[i] > 0)
					//{
					//	skip[i]--;
					//	continue;
					//}
					if (skip[i] < 0)
					{
						continue;
					}

					float f0a_ = f0a + (p[1].y - p[2].y) * (j - minx) * 1.0 / f12;
					float f0b_ = f0b + (p[2].y - p[0].y) * (j - minx) * 1.0 / f20;
					float f1a_ = f1a + (p[3].y - p[0].y) * (j - minx) * 1.0 / f30;
					float f1b_ = f1b + (p[0].y - p[2].y) * (j - minx) * 1.0 / f02;
					int color = check_point(j, i, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward, camera.forward,
					                        f0a_,
					                        f0b_,
					                        f1a_,
					                        f1b_
					);
					if (color >= 0)
					{
						*(buffer + i * WINDOW_WIDTH + j) |= color;
						has_printed[i] = 1;;
					}
					else
					{
						//*(buffer + i * WINDOW_WIDTH + j) = 0xFF;
						if (has_printed[i] > 0)
							skip[i] = -1;
						//else
						//{
						//	float f0a_2 = f0a_ + (p[1].y - p[2].y) * 16 * 1.0 / f12;
						//	float f0b_2 = f0b_ + (p[2].y - p[0].y) * 16 * 1.0 / f20;
						//	float f1a_2 = f1a_ + (p[3].y - p[0].y) * 16 * 1.0 / f30;
						//	float f1b_2 = f1b_ + (p[0].y - p[2].y) * 16 * 1.0 / f02;
						//	if (!(
						//		f0a_2 > -1e-4 && (1-f0a_2-f0b_2) > -1e-4 && f0b_2>-1e-4
						//		||
						//		f1a_2 > -1e-4 && (1 - f1a_2 - f1b_2)> -1e-4 && f1b_2>-1e-4
						//		)){
						//		skip[i] = 16;
						//	}
						//}
					}
				}
			}
		}
	}

	count += 1;

	static char tmp[100] = "FPS:\0";
	QueryPerformanceCounter(&t2);

	double frames = tf.QuadPart / (t2.QuadPart - t1.QuadPart);
	static double now_time = 0;
	now_time += 1.0 * (t2.QuadPart - t1.QuadPart) / tf.QuadPart;
	if (now_time >= 1.0)
	{
		QueryPerformanceFrequency(&tf);
		now_time = 0;
		sprintf(tmp, "FPS: %d\0", count);
		count = 0;
	}

	frames *= 0.4;
	frames += 40;

	// ��ת����
	// ���ݵ�ǰ֡���ж���ת����
	camera.forward = Eigen::Vector3f(0, 0, -1);
	camera.up = Eigen::Vector3f(0, 1, 0);
	camera.right = Eigen::Vector3f(-1, 0, 0);
	camera.position = Eigen::Vector3f(0, 0, 50);
	static float r_x = 0, r_y = 0;

	r_x += 1.0 * a_x * 360 / frames;
	r_y += 1.0 * a_y * 360 / frames;
	r_x = min(90, r_x);
	r_x = max(-90, r_x);
	float cx = cos(r_x / 180 * 3.14);
	float sx = sin(r_x / 180 * 3.14);
	float cy = cos(r_y / 180 * 3.14);
	float sy = sin(r_y / 180 * 3.14);
	float cz = cos(3.14 * (60.0 / frames) * a_z / 20);
	float sz = sin(3.14 * (60.0 / frames) * a_z / 20);
	rotate_x(camera.forward.data(), cx, sx);
	rotate_x(camera.up.data(), cx, sx);
	rotate_x(camera.right.data(), cx, sx);

	rotate_y(camera.right.data(), cy, sy);
	rotate_y(camera.forward.data(), cy, sy);
	rotate_y(camera.up.data(), cy, sy);

	rotate_z(camera.forward.data(), cz, sz);
	rotate_z(camera.up.data(), cz, sz);
	rotate_z(camera.right.data(), cz, sz);
	rotate_x(camera.position.data(), cx, sx);
	rotate_y(camera.position.data(), cy, sy);
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
	// �������ֱ���ɫ
	SetBkColor(Memhdc, RGB(0, 0, 0));
	// ����������ɫ
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
		// ��������
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
	//omp_set_num_threads(thread_count_b);
	Eigen::initParallel();
	__map_data = readBMP(map_name_str);

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
