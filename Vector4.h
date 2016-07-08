#pragma once
class Vector4
{
public:
    Vector4();
    Vector4(float x,float y,float z,float w);

    ~Vector4();
    Vector4 operator - (const Vector4 & target){
        return Vector4(vec[0] - target.vec[0], vec[1] - target.vec[1], vec[2] - target.vec[2], vec[3] - target.vec[3]);
    }

    float operator * (const Vector4 & target){
        return vec[0] * target.vec[0] + vec[1] * target.vec[1] + vec[2] * target.vec[2];
    }

    Vector4 operator / (const Vector4 & target){
        return Vector4(
            vec[1] * target.vec[2] - vec[2] * target.vec[1],
            vec[2] * target.vec[0] - vec[0] * target.vec[2],
            vec[0] * target.vec[1] - vec[1] * target.vec[0],
            0
            );
    }
    float vec[4];

};
