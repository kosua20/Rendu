#ifndef Joystick_h
#define Joystick_h

#include <glm/glm.hpp>
#include <map>

class Joystick {

public:

	Joystick(glm::vec3 & eye, glm::vec3 & center, glm::vec3 & up, glm::vec3 & right);

	~Joystick();
	
	void activate(int id);
	
	void deactivate();
	
	/// Reset the the joystick camera parameters.
	void reset();

	/// Update the values of the 4 view-frame vectors.
	void update(double elapsedTime);
	
	/// Joystick ID (or -1 if no joystick is connected).
	int id(){ return _id; }
	
private:
	
	enum Inputs {
		MOVE_FORWARD, MOVE_LATERAL, LOOK_VERTICAL, LOOK_LATERAL, MOVE_UP, MOVE_DOWN,
		RESET_ALL, RESET_CENTER, RESET_ORIENTATION, SPEED_UP, SPEED_DOWN
	};
	
	void configure();
	
	static std::string trim(const std::string & str, const std::string & del);
	
	/// Joystick ID (or -1 if no joystick is connected).
	int _id;
	
	glm::vec3 & _eye;
	glm::vec3 & _center;
	glm::vec3 & _up;
	glm::vec3 & _right;

	float _speed;
	float _angularSpeed;
	
	// References to GLFW flags.
	int _axisCount;
	int _buttonsCount;
	const float * _axes;
	const unsigned char * _buttons;
	std::map<Inputs, bool> _recentPress;
	std::map<Inputs, int> _codes;
	
};

#endif
