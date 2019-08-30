#pragma once

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>

#include "system/Logger.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/io.hpp>

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <map>
#include <algorithm>

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;

#ifdef _WIN32

#	undef near
#	undef far
#	define NOMINMAX
#	define M_PI 3.14159265358979323846
#	define M_PI_2 1.57079632679489661923
#endif

/**
 
 \defgroup Engine Engine
 \brief General code.
 
 \defgroup Graphics Graphics
 \brief Contain items and functions interacting with the graphics API.
 
 \defgroup Scene Scene
 \brief Contain objects, lights and environment representations used in 3D scene rendering.
 
 \defgroup Raycaster Raycaster
 \brief Compute intersection between rays and geometry.
 
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
 \brief Applications built with Rendu.
 
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
