#pragma once

#include "renderers/shadowmaps/ShadowMap.hpp"
#include "graphics/Framebuffer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"

/**
 \brief A 2D variance shadow map array, can be used for directional and spot lights. The shadow map will register itself with the associated lights. Implement variance shadow mapping to filter the shadows and get correct smoother edges.
 \ingroup Renderers
 */
class VarianceShadowMap2DArray : public ShadowMap {
public:
	/** Constructor.
	 \param lights the lights to generate the associated shadow maps for
	 \param resolution the shadow map resolution
	 */
	explicit VarianceShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights, const glm::vec2 & resolution);
	
	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) override;

private:
	
	std::vector<std::shared_ptr<Light>> _lights; ///< The associated light.
	Program * _program;			///< Shadow program.
	std::unique_ptr<Framebuffer> _map;	///< Raw shadow map result.
	std::unique_ptr<BoxBlur> _blur;		///< Blur filter.
	
};

/**
 \brief A cube variance shadow map array, can be used for point lights. Each face of the map is updated sequentially. The shadow map will register itself with the associated lights. Implement variance shadow mapping to filter the shadows and get correct smoother edges.
 \ingroup Renderers
 */
class VarianceShadowMapCubeArray : public ShadowMap {
public:
	/** Constructor.
	 \param lights the lights to generate the associated shadow maps for
	 \param side the shadow map resolution
	 */
	explicit VarianceShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights, int side);
	
	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) override;
	
private:
	
	std::vector<std::shared_ptr<PointLight>> _lights; ///< The associated lights.
	Program * _program;			///< Shadow program.
	std::unique_ptr<Framebuffer> _map;	///< Raw shadow map result.
	std::unique_ptr<BoxBlur> _blur;		///< Blur filter.
	
};
