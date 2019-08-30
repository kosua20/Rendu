#pragma once

#include "input/Camera.hpp"
#include "Common.hpp"

/** \brief This camera can be controlled through the inputs received from the user.
 \details It support turntable, fps and josytick based navigation methods, and handles their synchronization.
 \ingroup Input
 */
class ControllableCamera final : public Camera {
	
public:
	
	/// The interaction mode of the controllable camera.
	enum class Mode : uint {
		FPS = 0,
		TurnTable,
		//Trackball, ///< Currently not supported.
		Joystick
	};
	
	/// Constructor
	ControllableCamera();
	
	/** Update all view parameters
	 \param position the camera position
	 \param center the camera center of interest
	 \param up the camera vertical orientation
	 */
	void pose(const glm::vec3 & position, const glm::vec3 & center, const glm::vec3 & up) override;
	
	/// Reset the position of the camera.
	void reset();

	/// Update once-per-frame parameters.
	void update();
	
	/** Update the camera position and orientation.
	 \param frameTime the time elapsed since the last frame, in seconds.
	 */
	void physics(double frameTime);
	
	/** Access the speed parameter.
	 \return reference to the speed.
	 */
	float & speed(){ return _speed; }
	
	/** Obtain the mode the camera is currently using.
	 \return the current mode
	 */
	Mode & mode(){ return _mode; }
	
	
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
	
	float _speed = 1.2f; ///< Camera speed
	float _angularSpeed = 4.0f; ///< Camera angular speed
	
	// Camera additional state.
	glm::vec2 _angles = glm::vec2(float(M_PI)*0.5f, 0.0f); ///< Orientation angles
	float _radius = 1.0f; ///< Turntable radius
	
	Mode _mode = Mode::TurnTable; ///< The current interaction mode

};
