#pragma once
#ifndef TRANSFORM_H
#define TRANSFORM_H
#include "Vector4.h"

#define SIZE 20
struct Transform
{
public:
    //Transform(Vector4 position);
    Transform();
	Transform(Vector4& position);
	~Transform();

    Vector4 position;
    Vector4 forward, up, right;

    
};

#endif
