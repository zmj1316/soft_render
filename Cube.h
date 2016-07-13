#pragma once
#include "Transform.h"

class Reactangular
{
public:
    Reactangular()
    {
        vertexs[0] = Vector4(0, 0, 0, 1);
        vertexs[1] = Vector4(SIZE, 0, 0, 1);
        vertexs[2] = Vector4(SIZE, SIZE, 0, 1);
        vertexs[3] = Vector4(0, SIZE, 0, 1);
        transform.position = Vector4(SIZE / 2, -SIZE / 2, -SIZE / 2, 1);
        //transform.position = Vector4(0,0,0, 1);
    }
    Vector4 get_center()
    {
        Vector4 res;
        for (int i = 0; i < 3; ++i)
        {
            float sum = 0;
            for (int j = 0; j < 4; ++j)
            {
                sum += vertexs_world[j].vec[i];
            }
            res.vec[i] = sum / 4;
        }
        res.vec[3] = 0;
        return res;
    }

    float get_z()
    {
        Vector4 res;
        for (int i = 0; i < 3; ++i)
        {
            float sum = 0;
            for (int j = 0; j < 4; ++j)
            {
                sum += vertexs_view[j].vec[i];
            }
            res.vec[i] = sum / 4;
        }
        float sum = 0;
        for (int i = 0; i < 3; ++i)
        {
            sum += res.vec[i] * res.vec[i];
        }
        return sum;
    }

    Transform transform;
    Vector4 vertexs[4];
    Vector4 vertexs_world[4];
    Vector4 vertexs_view[4];
};


