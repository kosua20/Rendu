#ifndef Camera_h
#define Camera_h

#include <glm/glm.hpp>

class Camera {

public:

	Camera();

	~Camera();

	/// Reset the position of the camera.
	void reset();

	/// Update the view matrix.
	void update(float elapsedTime);

	/// Register a pressed or release movement key.
	void registerMove(int direction, bool flag);

	/// The view matrix.
	glm::mat4 _view;

private:
	glm::vec3 _eye;
	glm::vec3 _center;
	glm::vec3 _up;
	glm::vec3 _right;

	bool _keys[4];

	float _speed;

};

#endif
