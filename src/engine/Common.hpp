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
#define M_PI_2  1.57079632679489661923
#endif

#endif /* Common_hpp */

/**
 
 \defgroup Engine Engine
 \brief General code.
 
 \defgroup Graphics Graphics
 \brief Contain items and functions interacting with the graphics API.
 
 \defgroup Renderers Renderers
 \brief Renderer-specific implementations.
 
 \defgroup Lights Lights
 \brief Contain light objects used in 3D scene rendering.
 
 \defgroup Processing Processing
 \brief General image processing utilities.
 
 \defgroup Input Input
 \brief Handle user input through keyboard, mouse and controllers and provide controllable cameras.
 
 \defgroup Resources Resources
 \brief Handle all resources loading/saving tasks.
 
 \defgroup Helpers Helpers
 \brief Various utility helpers.
 
 \defgroup Shaders Shaders
 \brief OpenGL GLSL shaders.
 \details Those shaders are small programs compiled at runtime and executed by the GPU cores. They can process vertices (vertex shader), primitives (geometry shader) and compute per-pixel values (fragment shader).
 
 \defgroup Applications Applications
 \brief Applications built with GL_Template.
 
 \defgroup Tools Tools
 \brief Preprocess tools for shader validation, data precomputations, etc.
 
 \namespace GLSL
 \brief GLSL shaders
 
 \namespace GLSL::Vert
 \brief Contains all vertex shaders
 
 \namespace GLSL::Frag
 \brief Contains all fragment shaders
 
 \namespace GLSL::Geom
 \brief Contains all geometry shaders
 
 */
