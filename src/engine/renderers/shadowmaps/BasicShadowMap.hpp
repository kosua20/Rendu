#pragma once

#include "renderers/shadowmaps/ShadowMap.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"

/**
 \brief A 2D shadow map array, can be used for directional and spot lights. The shadow map will register itself with the associated lights.
 \ingroup Renderers
 */
class BasicShadowMap2DArray : public ShadowMap {
public:
	/** Constructor.
	 \param lights the lights to generate the associated shadow maps for
	 \param resolution the shadow map resolution
	 \param mode the type of shadow map technique used
	 */
	explicit BasicShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights, const glm::vec2 & resolution, ShadowMode mode);
	
	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) override;

private:
	
	std::vector<std::shared_ptr<Light>> _lights; ///< The associated light.
	Program * _program;			///< Shadow program.
	Texture _map;	///< Shadow map result.
	
};

/**
 \brief A cube shadow map array, can be used for point lights. Each face of the map is updated sequentially. The shadow map will register itself with the associated lights.
 \ingroup Renderers
 */
class BasicShadowMapCubeArray : public ShadowMap {
public:
	/** Constructor.
	 \param lights the lights to generate the associated shadow maps for
	 \param side the shadow map resolution
	 \param mode the type of shadow map technique used
	 */
	explicit BasicShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights, int side, ShadowMode mode);
	
	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) override;
	
private:
	
	std::vector<std::shared_ptr<PointLight>> _lights; ///< The associated lights.
	Program * _program;			///< Shadow program.
	Texture _map;	///< Shadow map result.
	
};


/**
 \brief A dummy shadow map array, can be used for directional and spot lights. The shadow map will register itself with the associated lights.
 \ingroup Renderers
 */
class EmptyShadowMap2DArray : public ShadowMap {
public:
	/** Constructor.
	 \param lights the lights to generate the associated shadow maps for
	 */
	explicit EmptyShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights);

	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) override;

private:

};

/**
 \brief A dummy cube shadow map array, can be used for point lights. The shadow map will register itself with the associated lights.
 \ingroup Renderers
 */
class EmptyShadowMapCubeArray : public ShadowMap {
public:
	/** Constructor.
	 \param lights the lights to generate the associated shadow maps for
	 */
	explicit EmptyShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights);

	/** \copydoc ShadowMap::draw  */
	void draw(const Scene & scene) override;

private:

};
