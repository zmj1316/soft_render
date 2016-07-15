#pragma once
#include <cmath>
#include "smmintrin.h"
struct Vector4
{
public:
	__declspec(align(16)) float vec[4];
	//__m128 vec_128;
    Vector4();
    Vector4(float x,float y,float z,float w);

    ~Vector4();

	bool mod_valid = false;
	float mod_f = 1;
	
	//void update_m()
	//{
	//	vec_128 = _mm_load_ps(vec);
	//}
	//void update_vec()
	//{
	//	_mm_store_ps(vec, vec_128);
	//}

	float mod()
	{

		if(!mod_valid)
		{
			auto t = _mm_load_ps(vec);

			float length = _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(t, t, 0x71)));
			mod_f = 1 / length;
			mod_valid = true;
		}

		return mod_f;
	}

	Vector4 normal()
	{
		mod();
		//return Vector4(vec[0] * mod_f, vec[1] * mod_f, vec[2] * mod_f, vec[3]);
		return mod_f*(*this);
	}

    Vector4 operator - (Vector4 & target){
		Vector4 res;
		auto t0 = _mm_load_ps(vec);
		auto t1 = _mm_load_ps(target.vec);
		//auto t2 = _mm_load_ps(res.vec);
		auto t3 = _mm_sub_ps(t0, t1);
		_mm_store_ps(res.vec, t3);
		return res;
        //return Vector4(vec[0] - target.vec[0], vec[1] - target.vec[1], vec[2] - target.vec[2], vec[3]);
    }

	static __m128 sp_add(float ss[], Vector4 vecs[])
	{
		return _mm_add_ps(_mm_add_ps((ss[0] * vecs[0]).get_m(), (ss[1] * vecs[1]).get_m()),(ss[2] * vecs[2]).get_m());
	}

	__m128 get_m() const
	{
		return _mm_load_ps(vec);
	}

	static __m128 sub(Vector4 & v0, Vector4 &v1)
	{
		auto t0 = _mm_load_ps(v0.vec);
		auto t1 = _mm_load_ps(v1.vec);
		return _mm_sub_ps(t0, t1);
	}

	static __m128 sub(__m128 & t0, Vector4 &v1)
	{
		//auto t0 = _mm_load_ps(v0.vec);
		auto t1 = _mm_load_ps(v1.vec);
		return _mm_sub_ps(t0, t1);
	}
	static __m128 add(Vector4 & v0, Vector4 &v1)
	{
		auto t0 = _mm_load_ps(v0.vec);
		auto t1 = _mm_load_ps(v1.vec);
		return _mm_add_ps(t0, t1);
	}

	static Vector4 get_vec(__m128 & m)
	{
		Vector4 res;
		_mm_store_ps(res.vec, m);
		return res;
	}

	static float mul(__m128 & v0, __m128 &v1)
	{
		return _mm_cvtss_f32(_mm_dp_ps(v0, v1, 0x71));
	}

	static float mod(__m128 & t)
	{
		return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(t, t, 0x71)));
	}

	Vector4 operator + (const Vector4 & target) const
	{
		Vector4 res;
		auto t0 = _mm_load_ps(vec);
		auto t1 = _mm_load_ps(target.vec);
		//auto t2 = _mm_load_ps(res.vec);
		auto t3 = _mm_add_ps(t0, t1);
		_mm_store_ps(res.vec, t3);
		return res;
		//Vector4 res;
		//res.vec[0] = vec[0] + target.vec[0];
		//res.vec[1] = vec[1] + target.vec[1];
		//res.vec[2] = vec[2] + target.vec[2];
		//res.vec[3] = vec[3] + target.vec[3];
		//return res;
	}
    float operator * (const Vector4 & target) const{
		auto t = _mm_load_ps(vec);
		auto t2 = _mm_load_ps(target.vec);
		return _mm_cvtss_f32(_mm_dp_ps(t, t2, 0x71));
    }



	friend Vector4 operator *(float r, const Vector4 & m) {
		//__declspec(align(16))  float t2[3] = { r, r, r };
		//auto t = _mm_load_ps(m.vec);
		//auto t1 = _mm_load_ps(t2);
		//auto rr = _mm_mul_ps(t, t1);
		//Vector4 res;
		//_mm_store_ps(res.vec, rr);


		return Vector4(r*m.vec[0], r*m.vec[1], r*m.vec[2], m.vec[3]);
	}
	// ���
    Vector4 operator / (const Vector4 & target) const {
        return Vector4(
            vec[1] * target.vec[2] - vec[2] * target.vec[1],
            vec[2] * target.vec[0] - vec[0] * target.vec[2],
            vec[0] * target.vec[1] - vec[1] * target.vec[0],
            0
            );
    }

	Vector4 operator-() {
		return Vector4(-vec[0], -vec[1], -vec[2], vec[3]);
	}
	//bool r_valid = false;
	//void do_r()
	//{
	//	if (!r_valid)
	//	{
	//		r_vec[0] = 1 / vec[0];
	//		r_vec[1] = 1 / vec[1];
	//		r_vec[2] = 1 / vec[2];

	//	}
	//}
	//float r_vec[4];
};
