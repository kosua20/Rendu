#pragma once
#include "system/Config.hpp"

class DirectionalLight;
class PointLight;
class SpotLight;

/**
 \brief Base structure of a per-light specialized renderer. It can be applied to a light by calling light->draw(renderer), which will then call the corresponding method below.
 \ingroup Engine
 */
class LightRenderer {

public:
	
	/** Perform a rendering task for a directional light.
	 \param light the light to visit.
	 */
	virtual void draw(const DirectionalLight * light) = 0;
	
	/** Perform a rendering task for a point light.
	 \param light the light to visit.
	 */
	virtual void draw(const PointLight * light) = 0;
	
	/** Perform a rendering task for a spot light.
	 \param light the light to visit.
	 */
	virtual void draw(const SpotLight * light) = 0;
	
	
};
