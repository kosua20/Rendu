#include "resources/Library.hpp"

const std::array<glm::vec3, 6> Library::boxUps = {
	glm::vec3(0.0f, -1.0f, 0.0f),
	glm::vec3(0.0f, -1.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, 1.0f),
	glm::vec3(0.0f, 0.0f, -1.0f),
	glm::vec3(0.0f, -1.0f, 0.0f),
	glm::vec3(0.0f, -1.0f, 0.0f)};

const std::array<glm::vec3, 6> Library::boxCenters = {
	glm::vec3(1.0, 0.0, 0.0),
	glm::vec3(-1.0, 0.0, 0.0),
	glm::vec3(0.0, 1.0, 0.0),
	glm::vec3(0.0, -1.0, 0.0),
	glm::vec3(0.0, 0.0, 1.0),
	glm::vec3(0.0, 0.0, -1.0)
};

const std::array<glm::vec3, 6> Library::boxRights = {
	glm::vec3( 0.0f, 0.0f, -1.0f),
	glm::vec3( 0.0f, 0.0f,  1.0f),
	glm::vec3( 1.0f, 0.0f,  0.0f),
	glm::vec3( 1.0f, 0.0f,  0.0f),
	glm::vec3( 1.0f, 0.0f,  0.0f),
	glm::vec3(-1.0f, 0.0f,  0.0f)
};

const std::array<glm::mat4, 6> Library::boxVs = {
	glm::lookAt(glm::vec3(0.0f), boxCenters[0], boxUps[0]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[1], boxUps[1]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[2], boxUps[2]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[3], boxUps[3]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[4], boxUps[4]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[5], boxUps[5])
};

const std::array<glm::mat4, 6> Library::boxVPs = {
	glm::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[0],
	glm::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[1],
	glm::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[2],
	glm::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[3],
	glm::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[4],
	glm::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[5]
};
