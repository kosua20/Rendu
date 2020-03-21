#pragma once

#include "scene/Scene.hpp"

#include "graphics/Framebuffer.hpp"
#include "processing/BoxBlur.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"

#include "Common.hpp"


/**
\brief Available shadow mapping techniques.
\ingroup DeferredRendering
*/
enum class ShadowMode : int {
	NONE	 = 0, ///< No shadows.
	BASIC	 = 1, ///< Basic depth test mode
	VARIANCE = 2  ///< Variance shadow map.
};

/**
 \brief Store shadowing information as a map generated from the light viewpoint.
 \ingroup DeferredRendering
 */
class ShadowMap {
public:
	/** Default constructor.*/
	ShadowMap() = default;
	
	/** Update the shadow map.
	 \param scene the objcts to draw in the map.
	 */
	virtual void draw(const Scene & scene) const = 0;
	
	/** Clean internal resources. */
	virtual void clean() = 0;
	
	/** Destructor. */
	virtual ~ShadowMap() = default;
	
	/** Copy constructor.*/
	ShadowMap(const ShadowMap &) = delete;
	
	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	ShadowMap & operator=(const Light &) = delete;
	
	/** Move constructor.*/
	ShadowMap(ShadowMap &&) = default;
	
	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	ShadowMap & operator=(ShadowMap &&) = delete;

};
