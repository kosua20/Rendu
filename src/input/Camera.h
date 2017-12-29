#ifndef Camera_h
#define Camera_h

#include <glm/glm.hpp>
#include "Keyboard.h"
#include "Joystick.h"

enum class MouseMode {
	Start, Move, End
};


class Camera {
	
public:
	
	Camera();

	~Camera();
	
	/// Reset the position of the camera.
	void reset();

	/// Update the view matrix.
	void update(double frameTime);

	/// Register a key press/release.
	void key(int key, bool flag);
	
	/// Register a joystick status change.
	void joystick(int joystick, int event);
	
	/// Register a mouse move or click.
	void mouse(MouseMode mode, float x, float y);
	
	/// Update the screen size and projection matrix.
	void screen(int width, int height);
	
	/// Update the internal vertical resolution.
	void internalResolution(int height);
	
	const glm::mat4 view() const { return _view; }
	const glm::mat4 projection() const { return _projection; }
	const glm::vec2 screenSize() const { return _screenSize; }
	const glm::vec2 renderSize() const { return _renderSize; }
	
private:
	
	void updateUsingJoystick(double frameTime);
	void updateUsingKeyboard(double frameTime);
	
	/// The view matrix.
	glm::mat4 _view;
	/// The projection matrix.
	glm::mat4 _projection;
	// Screen size
	glm::vec2 _screenSize;
	// Size use for render targets.
	glm::vec2 _renderSize;
	
	/// Vectors defining the view frame.
	glm::vec3 _eye;
	glm::vec3 _center;
	glm::vec3 _up;
	glm::vec3 _right;
	
	int _verticalResolution;
	
	float _speed;
	float _angularSpeed;
	
	

};

#endif
