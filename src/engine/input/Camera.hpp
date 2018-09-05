#ifndef Camera_h
#define Camera_h

#include "../Common.hpp"

/**
 \brief This class represents a camera as used in real-time rendering APIs.
 It provides a view and projection matrices, and handles their proper update.
 \ingroup Input
 */
class Camera {
	
public:
	
	/// Constructor
	Camera();
	
	/// Destructor
	~Camera();
	
	/** Update all projection parameters.
	 \param ratio the aspect ratio
	 \param fov the vertical field of view
	 \param near the near plane distance
	 \param far the far plane distance
	 */
	void projection(float ratio, float fov, float near, float far);
	
	/** Update the frustum near and far planes.
	 \param near the new near plane distance
	 \param far the new far plane distance
	 */
	void frustum(float near, float far);
	
	/** Update the aspect ratio.
	 \param ratio the new aspect ratio
	 */
	void ratio(float ratio);
	
	/** Update the vertical field of view.
	 \param fov the new field of view, in radians
	 */
	void fov(float fov);
	
	/**
	 Obtain the current vertical field of view.
	 \return the field of view
	 */
	float fov() const { return _fov; }
	
	/**
	 Obtain the current view matrix.
	 \return the view matrix
	 */
	const glm::mat4 view() const { return _view; }
	
	/**
	 Obtain the current projection matrix.
	 \return the projection matrix
	 */
	const glm::mat4 projection() const { return _projection; }
	
	/**
	 Obtain the current world space camera position.
	 \return the current position
	 */
	const glm::vec3 & position() const { return _eye; }
	
protected:
	
	/// Update the projection matrix using the camera parameters.
	void updateProjection();
	
	/// Update the view matrix using the camera position and orientation.
	void updateView();
	
	glm::mat4 _view; ///< The view matrix
	glm::mat4 _projection; ///< The projection matrix
	
	// Vectors defining the view frame.
	glm::vec3 _eye; ///< The camera position
	glm::vec3 _center; ///< The camera center (look-at point)
	glm::vec3 _up; ///< The camera up vector
	glm::vec3 _right; ///< The camera right vector
	
	float _fov; ///< The vertical field of view
	float _ratio; ///< The aspect ratio
	float _near; ///< The near plane distance
	float _far; ///< The far plane distance
	
};

#endif
