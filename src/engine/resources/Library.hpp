#pragma once

#include "resources/ResourcesManager.hpp"
#include "Common.hpp"
#include <array>


/**
 The library provides a few commonly-used resources without
 having to set them up or query them from the resources manager.
 \ingroup Resources
 */
class Library {

public:

	static const std::array<glm::vec3, 6> boxUps; ///< Skybox faces vertical direction.
	static const std::array<glm::vec3, 6> boxCenters; ///< Skybox faces center location.
	static const std::array<glm::vec3, 6> boxRights; ///< Skybox faces horizontal direction.
	static const std::array<glm::mat4, 6> boxVs; ///< Skybox faces view matrices.
	static const std::array<glm::mat4, 6> boxVPs; ///< Skybox faces view-projection matrices.
	
};

