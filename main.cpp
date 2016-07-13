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
static int sp_ = 3;					// specular
static float delta = 1e-5;			// delta
int map_width = -1;			//	size of bitmap
int map_height = -1;

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

Transform camera;



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
void getBilinearFilteredPixelColor(unsigned char tex[], float u, float v,unsigned char res[])
{
	u *= map_width;
	v *= map_height;
	int x = u;
	int y = v;
	float u_ratio = u - x;	
	float v_ratio = v - y;
	float u_opposite = 1 - u_ratio;
	float v_opposite = 1 - v_ratio;
	x = max(0, x);
	y = max(0, y);
	x = min(x, map_width-1);
	y = min(y, map_height-1);
	int x_d, y_d;
	x_d = min(x + 1, map_width - 1);
	y_d = min(y + 1, map_height - 1);

	//res[0] = tex[(x + y*map_height) * 3];
	//res[1] = tex[(x + y*map_height) * 3 + 1];
	//res[2] = tex[(x + y*map_height) * 3 + 2];
	res[0] = (tex[(x + y*map_height) * 3] * u_opposite + tex[(x_d + y*map_height) * 3] * u_ratio) * v_opposite + (tex[(x + (y_d)*map_height) * 3] * u_opposite + tex[(x_d + (y_d)*map_height) * 3] * u_ratio) * v_ratio;
	res[1] = (tex[(x + y*map_height) * 3 + 1] * u_opposite + tex[(x_d + y*map_height) * 3 + 1] * u_ratio) * v_opposite + (tex[(x + (y_d)*map_height) * 3 + 1] * u_opposite + tex[(x_d + (y_d)*map_height) * 3 + 1] * u_ratio) * v_ratio;
	res[2] = (tex[(x + y*map_height) * 3 + 2] * u_opposite + tex[(x_d + y*map_height) * 3 + 2] * u_ratio) * v_opposite + (tex[(x + (y_d)*map_height) * 3 + 2] * u_opposite + tex[(x_d + (y_d)*map_height) * 3 + 2] * u_ratio) * v_ratio;
}

#define _trans(new_vec,i) 	new_vec[i] = t.right.vec[i] * vec_src[0]\
							+ t.up.vec[i] * vec_src[1]\
							+ t.forward.vec[i] * vec_src[2]\
							+ t.position.vec[i] * vec_src[3]
// �任����
void transform(float* vec_src, float* vec_dst, Transform t)
{
	float new_vec[4];
	for (int i = 0; i < 4; i++)
	{
		new_vec[i] = t.right.vec[i] * vec_src[0]
			+ t.up.vec[i] * vec_src[1]
			+ t.forward.vec[i] * vec_src[2]
			+ t.position.vec[i] * vec_src[3];
	}
	//_trans(new_vec, 0);
	//_trans(new_vec, 1);
	//_trans(new_vec, 2);
	//_trans(new_vec, 3);
	memcpy(vec_dst, new_vec, 4 * sizeof(float));
}

// ����任
void WT(Reactangular& r0)
{
	for (int i = 0; i < 4; i++)
	{
		float* vec = r0.vertexs[i].vec;
		transform(vec, r0.vertexs_world[i].vec, r0.transform);
	}
}

// �ӽǱ任
void VT(Reactangular& r, Transform t)
{

	for (int i = 0; i < 4; i++)
	{
		float* vec = r.vertexs_world[i].vec;
		float* dst = r.vertexs_view[i].vec;
		transform(vec, dst, t);
		// ͸���ӽ� �������޸�
		dst[0] /= tan(3.14 / 4);
		dst[1] /= tan(3.14 / 4);
		dst[2] *= 0.5;
		// �������
		Vector4 t0 = r.vertexs_view[1] - r.vertexs_view[0];
		Vector4 t1 = r.vertexs_view[2] - r.vertexs_view[0];
		r.transform.forward = (t0 / t1).normal();
	}
}

// ͸��ͶӰ
void get_Pt(POINT* Pt, Reactangular& r0)
{

	for (int i = 0; i < 4; i++)
	{
		Pt[i].x = WINDOW_WIDTH/2 + 10 * r0.vertexs_view[i].vec[0] * 50 / r0.vertexs_view[i].vec[2];
		Pt[i].y = WINDOW_HEIGHT/2 - 10 * r0.vertexs_view[i].vec[1] * 50 / r0.vertexs_view[i].vec[2];
	}
}

// RGB ����
int do_RGB(float Rd, float Gd, float Bd)
{
	int R = min(255, Rd);
	int G = min(255, Gd);
	int B = min(255, Bd);
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

float f(const int a, const int b, const int x, const int y, const POINT p[])
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
	return (*a > -1e-7 && *a < 1+1e-7);
}

// ��դ��
// ����ֱ�Ӵ��������ν�������˷ָ��������������
float check_point(int x, int y, POINT p[], Vector4* vecs, Vector4& light_spot, Vector4 forward, Vector4 view_pos, int id)
{
	// ��ɫ
	unsigned char color[3];
	// ��������
	//int x0 = p[0].x;
	//int y0 = p[0].y;
	//float t1 = f(1, 2, x, y, p);
	//float t2 = f(1, 2, x0, y0, p);
	//float a = f(1, 2, x, y, p) / f(1, 2, p[0].x, p[0].y, p);
	float ss[3];
	ss[0] = f(1, 2, x, y, p) / f(1, 2, p[0].x, p[0].y, p);
	ss[1] = f(2, 0, x, y, p) / f(2, 0, p[1].x, p[1].y, p);
	ss[2] = 1 - ss[0] - ss[1];
	//float c = 1 - a - b;
	// �ж��Ƿ����ڲ�
	if (fast_judge(ss) && fast_judge(&ss[1]) && fast_judge(&ss[2]))
	{
		// �������
		Vector4 point = ss[0] * vecs[0] + ss[1] * vecs[1] + ss[2] * vecs[2];
		// ����
		Vector4 light = point - light_spot;
		// �ý��Ƽ��ټ���
		// ������
		float diff = light * forward * light_mod;
		diff = max(0, diff);
		// ȫ�ֹ�
		//diff += 0.1;
		// ����
		// mod()Ϊȡ���ȵ���
		Vector4 reflect = 2.0 * diff * (forward) - light_mod * light;
		//if (reflect_mod == 0)
			//reflect_mod = reflect.mod();
		// �ӽǷ���
		Vector4 view_dir = point - view_pos;
		float spec = my_pow(max(0, reflect*view_dir*view_dir.mod()*reflect.mod()), sp_);
		//min(spec, 1e10);
		// ͸�ӽ���
		float zr = ss[0] / vecs[0].vec[2] + ss[1] / vecs[1].vec[2] + ss[2]/ vecs[2].vec[2];
		float u = ((ss[0] * (0 / vecs[2].vec[2]) + ss[1] * (0 / vecs[1].vec[2]) + ss[2]*(1 / vecs[2].vec[2])) / zr);
		float v = ((ss[0] * (0 / vecs[2].vec[2]) + ss[1] * (1 / vecs[1].vec[2]) + ss[2]*(1 / vecs[2].vec[2])) / zr);

		// �������
		getBilinearFilteredPixelColor(map, u, v,color);
		return do_RGB((diff)* (color[0]) + ((spec)* (0xFF)),
			(diff)* (color[1]) + ((spec)* (0xFF)),
			(diff)* (color[2]) + ((spec)* (0xFF)));
	}
	// ������һ����������ͬ������
	else
	{
		ss[0]= f(3, 0, x, y, p) / f(3, 0, p[2].x, p[2].y, p);
		ss[1]= f(0, 2, x, y, p) / f(0, 2, p[3].x, p[3].y, p);
		ss[2] = 1 - ss[0] - ss[1];
		if (fast_judge(ss) && fast_judge(&ss[1]) && fast_judge(&ss[2]))
		{
			Vector4 point = ss[0] * vecs[2] + ss[1] * vecs[3] + ss[2]*vecs[0];
			Vector4 light = point - light_spot;
			float diff = light * forward * light_mod;
			diff = max(0, diff);
			//diff += 0.1;
			Vector4 reflect = 2.0 * diff * (forward) - light_mod * light;
			//if (reflect_mod == 0)
				//reflect_mod = reflect.mod();
			Vector4 view_dir = point - view_pos;
			float spec = my_pow(max(0, reflect*view_dir*view_dir.mod()*reflect.mod()), sp_);
			//min(spec, 1e10);

			float zr = ss[0] / vecs[2].vec[2] + ss[1] / vecs[3].vec[2] + ss[2]/ vecs[0].vec[2];
			float u = ((ss[0] * (1 / vecs[2].vec[2]) + ss[1] * (1 / vecs[3].vec[2]) + ss[2]*(0 / vecs[0].vec[2])) / zr);
			float v = ((ss[0] * (1 / vecs[2].vec[2]) + ss[1] * (0 / vecs[3].vec[2]) + ss[2]*(0 / vecs[0].vec[2])) / zr);
			//u = min(0.999, u);
			//u = max(0.001, u);
			//v = min(0.999, v);
			//v = max(0.001, v);
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

	camera.forward = Vector4(0, 0, -1, 0);
	camera.up = Vector4(0, 1, 0, 0);
	camera.right = Vector4(-1, 0, 0, 0);

}

void rotate_x(float* vec,float c,float s)
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
	static POINT Pt[4 * 6];
	static Zbuffer buffers[6];
	static int count = 0;

	// ��ʼ��ʱ
	QueryPerformanceCounter(&t1);

	// ����ͷ
	//camera.position = Vector4( -30- a_x, -4 + a_y, 50 - a_z, 1);
	camera.position = Vector4(0, 0, 50 - a_dep, 1);

	float cx = cos(3.14*a_x / 500);
	float sx = sin(3.14*a_x / 500);
	float cy = cos(3.14*a_y / 500);
	float sy = sin(3.14*a_y / 500);
	float cz = cos(3.14*a_z / 500);
	float sz = sin(3.14*a_z / 500);
	float cz2 = cos(3.14*a_z / 500);
	float sz2 = sin(3.14*a_z / 500);
	sz = 0;
	cz = 1;
	Transform cube;
	//cube.position = Vector4(0, 0, 0, 1);
	//cube.forward = Vector4(cx*sy*cz - sx*sz, -cx*sy*sz - sx*cz, -cx*cy, 0);
	//cube.up = Vector4(sx*sy*cz + cx*sz, -sx*sy*sz + cx*cz, -sx*cy, 0);
	//cube.right = Vector4(-cy*cz, sz*cy, -sy, 0);
	//camera.forward = Vector4(cx*sy*cz - sx*sz, -cx*sy*sz - sx*cz, -cx*cy, 0).normal();
	//camera.up = Vector4(sx*sy*cz + cx*sz, -sx*sy*sz + cx*cz, -sx*cy, 0).normal();
	//camera.right = Vector4(-cy*cz, sz*cy, -sy, 0).normal();

	//camera.forward = Vector4(0, 0, -1, 0);
	//camera.up = Vector4(0, 1, 0, 0);
	//camera.right = Vector4(-1, 0, 0, 0);

	//if (a_x > 0)
	{
		rotate_x(camera.forward.vec, cx, sx);
		rotate_x(camera.up.vec, cx, sx);
		rotate_x(camera.right.vec, cx, sx);

		rotate_y(camera.right.vec, cy, sy);
		rotate_y(camera.forward.vec, cy, sy);
		rotate_y(camera.up.vec, cy, sy);

		rotate_z(camera.forward.vec, cz2, sz2);
		rotate_z(camera.up.vec, cz2, sz2);
		rotate_z(camera.right.vec, cz2, sz2);
		//a_x-- ;
	}

	

	Vector4 light(0, 10, -40, 0);
	// ���ո�������ͷ
	// ȡ������ע�ͣ���������������
	//transform(light.vec, light.vec, camera); 

	// ��ÿ����
#pragma omp parallel for

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
		Vector4 view = camera.position - reacs[i].vertexs_view[0];
		// ???
		float* view_vec = view.vec;
		view_vec[0] /= tan(3.14 / 6);
		view_vec[1] /= tan(3.14 / 6);
		view_vec[2] *= 0.9;
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
		     return x.z < y.z;
	     });
	light_mod = light.mod();
	for (Zbuffer z : bsort)
	{
		if (z.z < 0)
			continue;
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
		//maxx = max(0, maxx);
		//maxy = max(0, maxy);
		//minx = min(WINDOW_WIDTH, minx);
		//miny = min(WINDOW_WIDTH, miny);
		minx = max(0, minx);
		miny = max(0, miny);
		// ɨ��
		int i, j;
		//reflect_mod = 0;
		for (i = miny; i < maxy; i++)
		{
#pragma omp parallel for
			for (j = minx; j < maxx / 2; j++){

				// ����Ѿ����˵�Ͳ�ɨ����
				if (*(buffer + i * WINDOW_WIDTH + j) > 0x0) continue;
				int color;
				if (z.i == 0 || z.i == 2)
				{
					color = check_point(j, i, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward, camera.forward, z.i);
				}
				else
					color = check_point(j, i, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward, camera.forward, z.i);
				if (color >= 0)
				{
					*(buffer + i * WINDOW_WIDTH + j) = color;
				}
			}
		}
		for (i = miny; i < maxy; i++)
		{
#pragma omp parallel for
			for (j = maxx / 2; j < maxx; j++){

				// ����Ѿ����˵�Ͳ�ɨ����
				if (*(buffer + i * WINDOW_WIDTH + j) > 0x0) continue;
				int color = check_point(j, i, p, reacs[z.i].vertexs_view, light, reacs[z.i].transform.forward, camera.forward, z.i);
				if (color >= 0)
				{
					*(buffer + i * WINDOW_WIDTH + j) = color;
				}
			}
		}

	}
	count += 1;

	static char tmp[100] = "FPS:\0";
	QueryPerformanceCounter(&t2);
	if (count == 10)
	{
		int frames = tf.QuadPart / (t2.QuadPart - t1.QuadPart);
		sprintf(tmp, "FPS: %d\0", frames);
		count = 0;
	}
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
			a_x--;
			break;
		case VK_RIGHT:
			a_x++;
			break;
		case VK_UP:
			a_y++;
			break;
		case VK_DOWN:
			a_y--;
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
