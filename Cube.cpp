#include "Cube.h"


Cube::Cube()
{
    transform = Transform(Vector4(1, 1, 1, 0));
    for (unsigned int i = 0; i < 8; i++)
    {
        vertexs[i] = Vector4((i & 4) >> 1, (i & 2), (i & 1) << 1, 1);
    }

    reactangular[0].transform.forward = Vector4(0, 0, 1, 0);
    reactangular[1].transform.forward = Vector4(0, 1, 0, 0);
    reactangular[2].transform.forward = Vector4(1, 0, 0, 0);
    reactangular[3].transform.forward = Vector4(0, 0, -1, 0);
    reactangular[4].transform.forward = Vector4(0, -1, 0, 0);
    reactangular[5].transform.forward = Vector4(-1, 0, 0, 0);

    reactangular[0].transform.position = Vector4(1, 1, 2, 0);
    reactangular[1].transform.position = Vector4(1, 2, 1, 0);
    reactangular[2].transform.position = Vector4(2, 1, 1, 0);
    reactangular[3].transform.position = Vector4(1, 1, 0, 0);
    reactangular[4].transform.position = Vector4(1, 0, 1, 0);
    reactangular[5].transform.position = Vector4(0, 1, 1, 0);

}


Cube::~Cube()
{
}