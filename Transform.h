#pragma once
#ifndef TRANSFORM_H
#define TRANSFORM_H
#include "Vector4.h"
#include "matrix.h"

#define SIZE 30
class Transform
{
public:
    Transform();
    Transform(Vector4 position);
    ~Transform();

    Vector4 position;
    Vector4 forward, up, right;

    
};

inline Vector4 tranform_trans(Transform transform, Vector4 vector)
{
    matrix<float> m(4, 4);



    return vector;
}
#endif
