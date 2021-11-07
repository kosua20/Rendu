#pragma once

#include "system/Codable.hpp"
#include "Common.hpp"

/**
 \brief This class represents a camera as used in real-time rendering APIs.
 It provides a view and projection matrices, and handles their proper update.
 \ingroup Input
 */
class Camera {

public:
	/// Constructor
	Camera();

	/** Update all view parameters
	 \param position the camera position
	 \param center the camera center of interest
	 \param up the camera vertical orientation
	 */
	virtual void pose(const glm::vec3 & position, const glm::vec3 & center, const glm::vec3 & up);

	/** Update all projection parameters.
	 \param ratio the aspect ratio
	 \param fov the vertical field of view in radians
	 \param near the near plane distance
	 \param far the far plane distance
	 */
	virtual void projection(float ratio, float fov, float near, float far);

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

	/** Compute world space position of the top-left corner of the image,
	 and the shifts to move to the next pixel, horizontally and vertically.
	 \param corner will contain the position of the top-left corner
	 \param dx will contain the horizontal step
	 \param dy will contain the vertical step
	 */
	void pixelShifts(glm::vec3 & corner, glm::vec3 & dx, glm::vec3 & dy) const;

	/**
	 Obtain the current vertical field of view, in radians.
	 \return the field of view
	 */
	float fov() const { return _fov; }

	/**
	 Obtain the current aspect ratio.
	 \return the aspect ratio
	 */
	float ratio() const { return _ratio; }

	/**
	 Obtain the current view matrix.
	 \return the view matrix
	 */
	const glm::mat4 & view() const { return _view; }

	/**
	 Obtain the current projection matrix.
	 \return the projection matrix
	 */
	const glm::mat4 & projection() const { return _projection; }

	/**
	 Obtain the current world space camera position.
	 \return the current position
	 */
	const glm::vec3 & position() const { return _eye; }

	/**
	 Obtain the current world space up direction.
	 \return the current direction
	 */
	const glm::vec3 & up() const { return _up; }

	/**
	 Obtain the current world space center position.
	 \return the current center
	 */
	const glm::vec3 & center() const { return _center; }

	/**
	 Obtain the current world space view direction.
	 \return the current direction
	 */
	const glm::vec3 direction() const { return glm::normalize(_center - _eye); }

	/**
	 Obtain the current world space camera position.
	 \return the current position
	 */
	const glm::vec2 & clippingPlanes() const { return _clippingPlanes; }

	/** Apply the pose and parameters of another camera.
	 \param camera the camera to align with
	 */
	void apply(const Camera & camera);

	/** Setup a camera parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 camera:
	 	position: X,Y,Z
	 	center: X,Y,Z
	 	up: X,Y,Z
	 	fov: F
	 	planes: N,F
	 \endverbatim
	 where the field of view is given in radians.
	 \param params the parameters tuples list
	 \return decoding status
	 */
	bool decode(const KeyValues & params);

	/** Encode a camera as a key-values representation.
	 \return the encoded camera parameters
	 */
	KeyValues encode() const;

	/** Destructor. */
	virtual ~Camera() = default;

	/** Copy constructor.*/
	Camera(const Camera &) = default;

	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	Camera & operator=(const Camera &) = default;

	/** Move constructor.*/
	Camera(Camera &&) = default;

	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	Camera & operator=(Camera &&) = default;

protected:
	/// Update the projection matrix using the camera parameters.
	void updateProjection();

	/// Update the view matrix using the camera position and orientation.
	void updateView();

	glm::mat4 _view		  = glm::mat4(1.0f); ///< The view matrix
	glm::mat4 _projection = glm::mat4(1.0f); ///< The projection matrix

	// Vectors defining the view frame.
	glm::vec3 _eye	= glm::vec3(0.0, 0.0, 1.0); ///< The camera position
	glm::vec3 _center = glm::vec3(0.0, 0.0, 0.0); ///< The camera center (look-at point)
	glm::vec3 _up	 = glm::vec3(0.0, 1.0, 0.0); ///< The camera up vector
	glm::vec3 _right  = glm::vec3(1.0, 0.0, 0.0); ///< The camera right vector

	glm::vec2 _clippingPlanes = glm::vec2(0.01f, 100.0f); ///< The near and far plane distances.

	float _fov   = 1.3f; ///< The vertical field of view, in radians.
	float _ratio = 1.0f; ///< The aspect ratio
};
