#include "Transform.h"


Transform::Transform()
{
    forward = Vector4(0, 0, 1, 0);
    right = Vector4(1, 0, 0, 0);
    up = Vector4(0, 1, 0, 0);

}


Transform::Transform(Vector4 position)
{
    this->position = position;
    Transform();
}

Transform::~Transform()
{
}

