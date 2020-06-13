#pragma once

#include "PathTracer.hpp"
#include "raycaster/RaycasterVisualisation.hpp"
#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"

#include "Common.hpp"

/**
 \brief Renderer coupled with a basic diffuse path tracer. The user can move the camera anywhere and trigger a path-traced rendering.
 Can also display the raycaster acceleration structure.
 \ingroup PathtracerDemo
 */
class BVHRenderer final : public Renderer {

public:
	/** Constructor.
	 */
	explicit BVHRenderer();

	/** Set the scene to render.
	 \param scene the new scene
	 \param raycaster the associated raycaster
	 */
	void setScene(const std::shared_ptr<Scene> & scene, const Raycaster & raycaster);

	/** \copydoc Renderer::draw */
	void draw(const Camera & camera, Framebuffer & framebuffer, size_t layer = 0) override;

	/** Destructor. */
	~BVHRenderer() override;

	/** Generate visualisation for a ray cast from a position.
	 \param position the ray origin.
	 \param direction the ray direction.
	 */
	void castRay(const glm::vec3 & position, const glm::vec3 & direction);
	
	/** Delete the displayed ray. */
	void clearRay();
	
	/** Show the BVH structure.
	 \return the toggle
	 */
	bool & showBVH(){ return _showBVH; }
	
	/** Display a range of levels of the BVH.
	 \return the range
	 */
	glm::ivec2 & range(){ return _bvhRange; }
	
	/** The maximum depth of the BVH.
	 \return the maximum level
	 */
	int maxLevel();
	
private:
	
	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Scene buffer.

	const Program * _objectProgram; ///< Basic object program.
	const Program * _bvhProgram;	///< BVH visualisation program.
	std::vector<Mesh> _bvhLevels;   ///< The BVH visualisation mesh.
	std::vector<Mesh> _rayLevels;   ///< BVH nodes intersected with a ray.
	Mesh _rayVis;					///< Mesh representing a ray and its intersected triangle.
	
	std::shared_ptr<Scene> _scene;						 ///< The scene to render.
	std::unique_ptr<RaycasterVisualisation> _visuHelper; ///< Helper for raycaster internal data visualisation.

	glm::ivec2 _bvhRange = glm::ivec2(0, 1); ///< The subset of the BVH to display.
	bool _showBVH		 = true;			 ///< Show the raytracer BVH.
};
