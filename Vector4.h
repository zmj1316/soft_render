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
			float sum = 0;
			for (size_t i = 0; i < 3; i++)
			{
				sum += vec[i] * vec[i];
			}
			mod_f = 1/sqrt(sum);
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
	Vector4 operator + (const Vector4 & target) {
		return Vector4(vec[0] + target.vec[0], vec[1] + target.vec[1], vec[2] + target.vec[2], vec[3]);
	}
    float operator * (const Vector4 & target) const{
        return vec[0] * target.vec[0] + vec[1] * target.vec[1] + vec[2] * target.vec[2];
    }

	friend Vector4 operator *(float r, const Vector4 & m) {
		return Vector4(r*m.vec[0], r*m.vec[1], r*m.vec[2], m.vec[3]);
	}
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

};
