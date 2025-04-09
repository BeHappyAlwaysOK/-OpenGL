#ifndef RECTANGLE_OBJECT_H
#define RECTANGLE_OBJECT_H

#include <math.h>
#include<glad/glad.h>
#include "maths.h"

namespace object3d {

    class rectangle {
    public:
        GLuint vao;

        rectangle(const vec3& p1, const vec3& p2);
    };

};

#endif
