#pragma once

#include "processing/BoxBlur.hpp"
#include "resources/Texture.hpp"

#include "Common.hpp"

class Scene;

/**
\brief Available shadow mapping techniques.
\ingroup Renderers
*/
enum class ShadowMode : int {
	NONE	 = 0, ///< No shadows.
	BASIC	 = 1, ///< Basic depth test mode
	PCF	 	 = 2, ///< Basic depth test mode
	VARIANCE = 3  ///< Variance shadow map.
};

/**
 \brief Store shadowing information as a map generated from the light viewpoint.
 \ingroup Renderers
 */
class ShadowMap {
public:
	/** Default constructor.*/
	ShadowMap() = default;
	
	/** Update the shadow map.
	 \param scene the objcts to draw in the map.
	 */
	virtual void draw(const Scene & scene) = 0;
	
	/** Destructor. */
	virtual ~ShadowMap() = default;
	
	/** Copy constructor.*/
	ShadowMap(const ShadowMap &) = delete;
	
	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	ShadowMap & operator=(const ShadowMap &) = delete;
	
	/** Move constructor.*/
	ShadowMap(ShadowMap &&) = default;
	
	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	ShadowMap & operator=(ShadowMap &&) = delete;

	/** Define a region in a 2D or array texture, containing a shadow map content. */
	struct Region {
		const Texture * map = nullptr; ///< The texture reference.
		ShadowMode mode = ShadowMode::NONE; ///< The shadow mode to use.
		glm::vec2 minUV = glm::vec2(0.0f); ///< The bottom-left corner of the texture region.
		glm::vec2 maxUV = glm::vec2(0.0f); ///< The upper-right corner of the texture region.
		size_t layer = 0; ///< The layer containing the shadow map.
		float bias = 0.002f; ///< The depth bias to use.
	};

};
