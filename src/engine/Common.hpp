#pragma once

#ifdef _WIN32
#	define NOMINMAX
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/constants.hpp>

#include <gl3w/gl3w.h>
#include <imgui/imgui.h>

#include <string>
#include <vector>
#include <algorithm>

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned long ulong;

#ifdef _WIN32
#	undef near
#	undef far
#	undef ERROR
#endif

#include "system/Logger.hpp"

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
 
 \defgroup System System
 \brief Interaction with the operating system.
 
 \defgroup Shaders Shaders
 \brief Vertex processing and shading subprograms.
 \details Those shaders are small programs compiled at runtime and executed by the GPU cores. They can process vertices (vertex shader), primitives (geometry shader) and compute per-pixel values (fragment shader).
 
 \defgroup Applications Applications
 \brief Applications built with Rendu.
 
 \defgroup Tools Tools
 \brief Preprocess tools for shader validation, data precomputations, etc.
 
 \namespace GPU
 \brief GPU shaders
 
 \namespace GPU::Vert
 \brief Contains all vertex shaders
 
 \namespace GPU::Frag
 \brief Contains all fragment shaders
 
 \namespace GPU::Geom
 \brief Contains all geometry shaders
 
 */
