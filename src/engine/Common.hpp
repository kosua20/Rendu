#pragma once

#ifdef _WIN32
#	define NOMINMAX
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/constants.hpp>

#include <imgui/imgui.h>

#include <string>
#include <vector>
#include <algorithm>
#include <memory>

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned long ulong;

#ifdef _WIN32
#	undef near
#	undef far
#	undef ERROR
#endif

#include "system/Logger.hpp"
