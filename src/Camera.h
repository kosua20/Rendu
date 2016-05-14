#ifndef Camera_h
#define Camera_h

#include <glm/glm.hpp>

class Camera {

public:

	Camera();

	~Camera();

	void update(float elapsedTime);

	glm::mat4 _view;

private:
	glm::vec3 _eye;
	glm::vec3 _center;
	glm::vec3 _up;

};

#endif
