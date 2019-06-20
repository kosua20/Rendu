#pragma once

#include "scene/Codable.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/** \brief An animation is a transformation evaluated at each frame and applied to an object.
 \ingroup Scene
 */
class Animation {
public:
	
	/** \brief Frame in which the transformation shoud be applied. */
	enum class Frame {
		MODEL, ///< Model space (right multiplication)
		WORLD  ///< World space (left multiplication)
	};
	
	/** Apply the animation transformation to the input matrix.
	 \param m the matrix to transform
	 \param fullTime the time elapsed since the beginning of the rendering loop
	 \param frameTime the time elapsed since last frame
	 */
	virtual glm::mat4 apply(const glm::mat4 & m, double fullTime, double frameTime) = 0;
	
	/** Apply the animation transformation to the input vector.
	 \param v the vector to transform
	 \param fullTime the time elapsed since the beginning of the rendering loop
	 \param frameTime the time elapsed since last frame
	 */
	virtual glm::vec4 apply(const glm::vec4 & v, double fullTime, double frameTime) = 0;
	
	/** Virtual destructor. */
	virtual ~Animation(){};
	
	static std::shared_ptr<Animation> decode(const KeyValues & params);
	
protected:
	
	void decodeBase(const KeyValues & params);
	
	Frame _frame = Frame::WORLD; ///< The frame of transformation.
	float _speed = 0.0f; ///< Speed of the animation.
};

/** \brief Rotate an object around an axis.
 \ingroup Scene
 */
class Rotation : public Animation {
public:
	
	/** Default constructor. */
	Rotation();
	
	/** Setup a rotation animation.
	 \param axis the rotation axis
	 \param speed the animation speed
	 \param frame the animation frame
	 */
	Rotation(const glm::vec3 & axis, float speed, Frame frame);
	
	/** Apply the animation transformation to the input matrix.
	 \param m the matrix to transform
	 \param fullTime the time elapsed since the beginning of the rendering loop
	 \param frameTime the time elapsed since last frame
	 */
	glm::mat4 apply(const glm::mat4 & m, double fullTime, double frameTime);
	
	/** Apply the animation transformation to the input vector.
	 \param v the vector to transform
	 \param fullTime the time elapsed since the beginning of the rendering loop
	 \param frameTime the time elapsed since last frame
	 */
	glm::vec4 apply(const glm::vec4 & v, double fullTime, double frameTime);
	
	void decode(const KeyValues & params);
	
private:
	
	glm::vec3 _axis = glm::vec3(1.0f, 0.0f, 0.0f); ///< Rotation axis.
};

/** \brief Translate an object back and forth along a direction.
 \ingroup Scene
 */
class BackAndForth : public Animation {
public:
	
	/** Default constructor. */
	BackAndForth();
	
	/** Setup a back and forth animation.
	 \param axis the translation direction
	 \param speed the animation speed
	 \param amplitude the amplitude of the movement
	 \param frame the animation frame
	 */
	BackAndForth(const glm::vec3 & axis, float speed, float amplitude, Frame frame);
	
	/** Apply the animation transformation to the input matrix.
	 \param m the matrix to transform
	 \param fullTime the time elapsed since the beginning of the rendering loop
	 \param frameTime the time elapsed since last frame
	 */
	glm::mat4 apply(const glm::mat4 & m, double fullTime, double frameTime);
	
	/** Apply the animation transformation to the input vector.
	 \param v the vector to transform
	 \param fullTime the time elapsed since the beginning of the rendering loop
	 \param frameTime the time elapsed since last frame
	 */
	glm::vec4 apply(const glm::vec4 & v, double fullTime, double frameTime);
	
	void decode(const KeyValues & params);
	
private:
	
	glm::vec3 _axis = glm::vec3(1.0f, 0.0f, 0.0f); ///< Translation direction.
	float _amplitude = 0.0f; ///< Amplitude of the translation (maximum distance).
	float _previousAbscisse = 0.0f; ///< Position on the path at the previous frame.
};
