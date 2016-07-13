#pragma once
#include <cmath>

class Vector4
{
public:
    Vector4();
    Vector4(float x,float y,float z,float w);

    ~Vector4();

	bool mod_valid = false;
	float mod_f = 1;

	float mod()
	{
		if(!mod_valid)
		{
			mod_f = 1 / sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
			mod_valid = true;
		}

		return mod_f;
	}

	Vector4 normal()
	{
		mod();
		return Vector4(vec[0] * mod_f, vec[1] * mod_f, vec[2] * mod_f, vec[3]);
	}

    Vector4 operator - (const Vector4 & target){
        return Vector4(vec[0] - target.vec[0], vec[1] - target.vec[1], vec[2] - target.vec[2], vec[3]);
    }
	Vector4 operator + (const Vector4 & target) const
	{
		Vector4 res;
		res.vec[0] = vec[0] + target.vec[0];
		res.vec[1] = vec[1] + target.vec[1];
		res.vec[2] = vec[2] + target.vec[2];
		res.vec[3] = vec[3] + target.vec[3];
		return res;
	}
    float operator * (const Vector4 & target) const{
        return vec[0] * target.vec[0] + vec[1] * target.vec[1] + vec[2] * target.vec[2];
    }

	friend Vector4 operator *(float r, const Vector4 & m) {
		return Vector4(r*m.vec[0], r*m.vec[1], r*m.vec[2], m.vec[3]);
	}
	// ²æ»ý
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
	float vec[4];
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
