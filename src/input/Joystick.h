#ifndef Joystick_h
#define Joystick_h

#include <glm/glm.hpp>
#include <map>

class Joystick {

public:

	Joystick();

	~Joystick();
	
	bool activate(int id);
	
	void deactivate();
	

	/// Update the values for axes and buttons.
	void update();
	
	
private:
	
	enum Inputs {
		MOVE_FORWARD, MOVE_LATERAL, LOOK_VERTICAL, LOOK_LATERAL, MOVE_UP, MOVE_DOWN,
		RESET_ALL, RESET_CENTER, RESET_ORIENTATION, SPEED_UP, SPEED_DOWN
	};
	
	bool configure();
	
	static std::string trim(const std::string & str, const std::string & del);
	
	/// Joystick ID (or -1 if no joystick is connected).
	int _id;
	
	// References to GLFW flags.
	int _axisCount;
	int _buttonsCount;
	const float * _axes;
	const unsigned char * _buttons;
	std::map<Inputs, bool> _recentPress;
	std::map<Inputs, int> _codes;
	
};

#endif
