#ifndef Camera_h
#define Camera_h

#include <glm/glm.hpp>

class Camera {
	
public:
	
	Camera();

	~Camera();
	
	/// Reset the position of the camera.
	void reset();

	/// Update one-shot parameters.
	void update();
	
	/// Update the view matrix.
	void physics(double frameTime);
	
	/// Update the screen size and projection matrix.
	void screen(int width, int height);
	
	/// Update all projection parameters.
	void projection(float ratio, float fov, float near, float far);
	
	/// Update the frustum near and far planes.
	void frustum(float near, float far);
	
	/// Update the aspect ratio.
	void ratio(float ratio);
	
	/// Update the FOV (in radians).
	void fov(float fov);
	
	float fov() const { return _fov; }
	
	const glm::mat4 view() const { return _view; }
	const glm::mat4 projection() const { return _projection; }
	
private:
	
	void updateUsingJoystick(double frameTime);
	
	void updateUsingKeyboard(double frameTime);
	
	void updateUsingTurnTable(double frameTime);
	
	/// Update the projection matrice parameters.
	void updateProjection();
	
	/// The view matrix.
	glm::mat4 _view;
	/// The projection matrix.
	glm::mat4 _projection;
	
	/// Vectors defining the view frame.
	glm::vec3 _eye;
	glm::vec3 _center;
	glm::vec3 _up;
	glm::vec3 _right;
	
	float _fov;
	float _ratio;
	float _near;
	float _far;
	float _speed;
	float _angularSpeed;
	
	// TurnTable parameters.
	float _verticalAngle;
	float _horizontalAngle;
	float _radius;
	
	enum CameraMode {
		FPS, TurnTable, Trackball
	};
	
	CameraMode _mode;
};

#endif
