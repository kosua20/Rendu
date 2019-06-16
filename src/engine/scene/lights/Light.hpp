#pragma once

#include "scene/Animation.hpp"
#include "Common.hpp"

/**
 \brief A general light with adjustable color intensity, that can cast shadows.
 \ingroup Scene
 */
class Light {

public:
	
	/** Constructor
	 \param color the light color intensity
	 */
	Light(const glm::vec3& color);
	
	/** Set if the light shoud cast shadows.
	 \param shouldCast toggle shadow casting
	 */
	void castShadow(const bool shouldCast){ _castShadows = shouldCast; }
	
	/** Set the light colored intensity.
	 \param color the new intensity to use
	 */
	void setIntensity(const glm::vec3 & color){ _color = color; }
	
	/** Add an animation to the light.
	 \param anim the animation to apply
	 */
	void addAnimation(std::shared_ptr<Animation> anim);
	
protected:
	
	glm::mat4 _mvp; ///< MVP matrix for shadow casting.
	glm::vec3 _color; ///< Colored intensity.
	bool _castShadows; ///< Is the light casting shadows (and thus use a shadow map).
	std::vector<std::shared_ptr<Animation>> _animations; ///< Animations list (will be applied in order).
};


inline Light::Light(const glm::vec3& color){
	_castShadows = false;
	_color = color;
	_mvp = glm::mat4(1.0f);
}

inline void Light::addAnimation(std::shared_ptr<Animation> anim){
	_animations.push_back(anim);
}


