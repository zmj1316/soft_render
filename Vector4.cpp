#include "Vector4.h"

Vector4::Vector4()
{
    
}

Vector4::Vector4(float x, float y, float z, float w)
{
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
    vec[3] = w;
	//vec_128 = _mm_load_ps(vec);
}


Vector4::~Vector4()
{
}
