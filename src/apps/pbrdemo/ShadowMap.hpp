#pragma once

#include "scene/Scene.hpp"

#include "graphics/Framebuffer.hpp"
#include "processing/BoxBlur.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"

#include "Common.hpp"

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

/**
 \brief A 2D shadow map, can be used for directional and spot lights. The shadow map will register itself with the associated light. Implement variance shadow mapping to filter the shadows and get correct smoother edges.
 \ingroup DeferredRendering
 */
class ShadowMap2D : public ShadowMap {
public:
	/** Constructor.
	 \param light the light to generate the associated shadow map for
	 \param resolution the shadow map resolution
	 */
	explicit ShadowMap2D(const std::shared_ptr<Light> & light, const glm::vec2 & resolution);
	
	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) const override;
	
	/** \copydoc ShadowMap::clean */
	void clean() override;

private:
	
	std::shared_ptr<Light> _light; 		///< The associated light.
	const Program * _program;			///< Shadow program.
	std::unique_ptr<Framebuffer> _map;	///< Raw shadow map result.
	std::unique_ptr<BoxBlur> _blur;		///< Filtered shadow map result.
	
};

/**
 \brief A cube shadow map, can be used for lights. Each face of the map is updated at the same time using a layered approach. The shadow map will register itself with the associated light.
 \ingroup DeferredRendering
 */
class ShadowMapCube : public ShadowMap {
public:
	/** Constructor.
	 \param light the light to generate the associated shadow map for
	 \param side the shadow map resolution
	 */
	explicit ShadowMapCube(const std::shared_ptr<PointLight> & light, int side);
	
	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) const override;
	
	/** \copydoc ShadowMap::clean */
	void clean() override;
	
private:
	
	std::shared_ptr<PointLight> _light;		///< The associated light.
	const Program * _program;				///< Shadow program.
	std::unique_ptr<FramebufferCube> _map;	///< Raw shadow map result.
	
};
