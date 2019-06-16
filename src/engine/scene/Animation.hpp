#pragma once
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"


class Animation {
public:
	
	enum class Frame {
		MODEL, WORLD
	};
	
	virtual glm::mat4 apply(const glm::mat4 & m, double fullTime, double frameTime) = 0;
	
	virtual glm::vec4 apply(const glm::vec4 & v, double fullTime, double frameTime) = 0;
	
};

class Rotation : public Animation {
public:
	
	Rotation(const glm::vec3 & axis, float speed, const Frame frame);
	
	glm::mat4 apply(const glm::mat4 & m, double fullTime, double frameTime);
	
	glm::vec4 apply(const glm::vec4 & v, double fullTime, double frameTime);
	
private:
	glm::vec3 _axis = glm::vec3(1.0f, 0.0f, 0.0f);
	float _speed = 0.0f;
	Frame _frame = Frame::WORLD;
	
};

class BackAndForth : public Animation {
public:
	
	BackAndForth(const glm::vec3 & axis, float speed, float amplitude, const Frame frame);
	
	glm::mat4 apply(const glm::mat4 & m, double fullTime, double frameTime);
	
	glm::vec4 apply(const glm::vec4 & v, double fullTime, double frameTime);
	
private:
	glm::vec3 _axis = glm::vec3(1.0f, 0.0f, 0.0f);
	float _speed = 0.0f;
	float _amplitude = 0.0f;
	Frame _frame = Frame::WORLD;
	float _previousAbscisse = 0.0f;
};
