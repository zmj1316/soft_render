#pragma once
#include "Transform.h"

class Reactangular
{
public:
    Reactangular()
    {
        vertexs[0] = Vector4(0, 0, 0, 1);
        vertexs[1] = Vector4(10, 0, 0, 1);
        vertexs[2] = Vector4(10, 10, 0, 1);
        vertexs[3] = Vector4(0, 10, 0, 1);
        transform.position = Vector4(22, 22, 22, 1);
    }

    Transform transform;
    Vector4 vertexs[4];
};

class Cube
{
public:
    Cube();
    ~Cube();

    Transform transform;
    Vector4 vertexs[8];
    Reactangular reactangular[6];
};

