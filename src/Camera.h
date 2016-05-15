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

	void startLeftMouse(double x, double y);
	void leftMouseTo(double x, double y);
	void endLeftMouse();

	/// The view matrix.
	glm::mat4 _view;

private:
	glm::vec3 _eye;
	glm::vec3 _center;
	glm::vec3 _up;
	glm::vec3 _right;

	bool _keys[7];

	float _speed;
	float _angularSpeed;
	
	glm::vec2 _previousPosition;
	glm::vec2 _deltaPosition;

};

#endif
