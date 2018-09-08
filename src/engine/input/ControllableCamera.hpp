#ifndef ControllableCamera_h
#define ControllableCamera_h

#include "Camera.hpp"
#include "../Common.hpp"

/** \brief This camera can be controlled through the inputs received from the user.
 \details It support turntable, fps and josytick based navigation methods, and handles their synchronization.
 \ingroup Input
 */
class ControllableCamera : public Camera {
	
public:
	
	/// Constructor
	ControllableCamera();
	
	/// Reset the position of the camera.
	void reset();

	/// Update once-per-frame parameters.
	void update();
	
	/** Update the camera position and orientation.
	 \param frameTime the time elapsed since the last frame, in seconds.
	 */
	void physics(double frameTime);

	
private:
	
	/** Update the camera state based on joystick inputs.
	 \param frameTime the time elapsed since the last frame, in seconds.
	 */
	void updateUsingJoystick(double frameTime);
	
	/** Update the camera state based on keyboard inputs, as a FPS camera.
	 \param frameTime the time elapsed since the last frame, in seconds.
	 */
	void updateUsingKeyboard(double frameTime);
	
	/** Update the camera state based on keyboard inputs, as a turntable.
	 \param frameTime the time elapsed since the last frame, in seconds.
	 */
	void updateUsingTurnTable(double frameTime);
	
	float _speed; ///< Camera speed
	float _angularSpeed; ///< Camera angular speed
	
	// Camera additional state.
	glm::vec2 _angles; ///< Orientation angles
	float _radius; ///< Turntable radius
	
	/// The interaction mode of the controllable camera.
	enum CameraMode {
		FPS,
		TurnTable,
		Trackball, ///< Currently not supported.
		Joystick
	};
	
	CameraMode _mode; ///< The current interaction mode
};

#endif
