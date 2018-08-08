//
//  Common.hpp
//  GL_Template
//
//  Created by Simon Rodriguez on 08/08/2018.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

#ifndef Common_hpp
#define Common_hpp

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>

#include "helpers/Logger.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#ifdef _WIN32

#undef near
#undef far
#define NOMINMAX
#define M_PI	3.14159265358979323846

#endif

#endif /* Common_hpp */
