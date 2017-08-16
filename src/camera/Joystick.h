#ifndef Joystick_h
#define Joystick_h

#include <glm/glm.hpp>


class Joystick {

public:

	Joystick(glm::vec3 & eye, glm::vec3 & center, glm::vec3 & up, glm::vec3 & right);

	~Joystick();
	
	void activate(int id);
	
	void deactivate();
	
	/// Reset the the joystick camera parameters.
	void reset();

	/// Update the values of the 4 view-frame vectors.
	void update(float elapsedTime);
	
	/// Joystick ID (or -1 if no joystick is connected).
	int id(){ return _id; }
	
private:
	
	/// Joystick ID (or -1 if no joystick is connected).
	int _id;
	
	glm::vec3 & _eye;
	glm::vec3 & _center;
	glm::vec3 & _up;
	glm::vec3 & _right;

	float _speed;
	float _angularSpeed;
	
};

#endif
