#pragma once

#include "renderers/shadowmaps/ShadowMap.hpp"
#include "graphics/Framebuffer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"

/**
 \brief A 2D variance shadow map, can be used for directional and spot lights. The shadow map will register itself with the associated light. Implement variance shadow mapping to filter the shadows and get correct smoother edges.
 \ingroup Renderers
 */
class VarianceShadowMap2D : public ShadowMap {
public:
	/** Constructor.
	 \param light the light to generate the associated shadow map for
	 \param resolution the shadow map resolution
	 */
	explicit VarianceShadowMap2D(const std::shared_ptr<Light> & light, const glm::vec2 & resolution);
	
	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) const override;

private:
	
	std::shared_ptr<Light> _light; 		///< The associated light.
	const Program * _program;			///< Shadow program.
	std::unique_ptr<Framebuffer> _map;	///< Raw shadow map result.
	std::unique_ptr<BoxBlur> _blur;		///< Blur filter.
	
};

/**
 \brief A cube variance shadow map, can be used for lights. Each face of the map is updated sequentially. The shadow map will register itself with the associated light. Implement variance shadow mapping to filter the shadows and get correct smoother edges.
 \ingroup Renderers
 */
class VarianceShadowMapCube : public ShadowMap {
public:
	/** Constructor.
	 \param light the light to generate the associated shadow map for
	 \param side the shadow map resolution
	 */
	explicit VarianceShadowMapCube(const std::shared_ptr<PointLight> & light, int side);
	
	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) const override;
	
private:
	
	std::shared_ptr<PointLight> _light;		///< The associated light.
	const Program * _program;				///< Shadow program.
	std::unique_ptr<Framebuffer> _map;	///< Raw shadow map result.
	std::unique_ptr<BoxBlur> _blur;		///< Blur filter.
	
};
