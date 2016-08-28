#ifndef Light_h
#define Light_h

#include <glm/glm.hpp>


enum class LightType {
	Directional, Point,
};

struct LightInternal
{
	glm::vec4 _p;
	glm::vec4 _Ia;
	glm::vec4 _Id;
	glm::vec4 _Is;
	float _shininess;
	
	
	LightInternal() { }
	
	LightInternal(const glm::vec4& p, const glm::vec4& Ia, const glm::vec4& Id, const glm::vec4& Is, const float shininess) : _p(p), _Ia(Ia), _Id(Id), _Is(Is), _shininess(shininess){ }
};


class Light {

public:

	Light();
	
	Light(const glm::vec4& local, const glm::vec4& Ia, const glm::vec4& Id, const glm::vec4& Is, const float shininess, const glm::mat4 projectionMatrix);
	
	~Light();
	
	void update(float time, const glm::mat4& camViewMatrix);
	
	const LightInternal* getStruct(){ return &_lightStruct; }
	
	glm::mat4 _mvp;
	
private:
	
	LightInternal _lightStruct;
	glm::mat4 _projectionMatrix;
	glm::mat4 _viewMatrix;
	
	glm::vec3 _local;
	LightType _type;
	
};

#endif
