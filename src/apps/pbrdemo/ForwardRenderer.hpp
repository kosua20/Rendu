#pragma once

#include "ForwardLight.hpp"

#include "renderers/DebugLightRenderer.hpp"

#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "renderers/Culler.hpp"

#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"

#include "processing/SSAO.hpp"

#include "Common.hpp"


/**
 \brief A renderer that shade each object as it is drawn in the scene directly.
 \sa ForwardLight
 \ingroup PBRDemo
 */
class ForwardRenderer final : public Renderer {

public:
	/** Constructor.
	 \param resolution the initial rendering resolution
	 \param mode the shadow rendering algorithm
	 \param ssao should screen space ambient occlusion be computed
	 \param name the debug name
	 */
	explicit ForwardRenderer(const glm::vec2 & resolution, ShadowMode mode, bool ssao, const std::string & name);

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** \copydoc Renderer::draw */
	void draw(const Camera & camera, Framebuffer & framebuffer, uint layer = 0) override;

	/** \copydoc Renderer::resize
	 */
	void resize(uint width, uint height) override;

	/** \copydoc Renderer::interface */
	void interface() override;

	/** \return the framebuffer containing the scene depth information */
	const Framebuffer * sceneDepth() const;

	/** \return the texture containing the scene normal information */
	const Texture * sceneNormal() const;
	
private:

	/** Render the scene object depth (prepass).
	 \param visibles list of indices of visible objects
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 \note Transparent and parallax objects will be skipped.
	 */
	void renderDepth(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj);

	/** Render the scene opaque objects.
	 \param visibles list of indices of visible objects
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 */
	void renderOpaque(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj);

	/** Render the scene transparent objects.
	 \param visibles list of indices of visible objects 
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 */
	void renderTransparent(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj);

	/** Render the scene background.
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 \param pos the camera position
	 */
	void renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos);

	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Scene framebuffer
	std::unique_ptr<SSAO> _ssaoPass;				///< SSAO processing.
	std::unique_ptr<ForwardLight> _lightsGPU;	///< The lights renderer.
	
	Program * _objectProgram;		 ///< Basic PBR program
	Program * _parallaxProgram;	 ///< Parallax mapping PBR program
	Program * _emissiveProgram;	 ///< Parallax mapping PBR program
	Program * _transparentProgram;	 ///< Transparent PBR program
	Program * _depthPrepass; ///< Depth prepass program.

	Program * _skyboxProgram; ///< Skybox program.
	Program * _bgProgram;		///< Planar background program.
	Program * _atmoProgram;   ///< Atmospheric scattering program.

	const Texture * _textureBrdf; ///< The BRDF lookup table.

	std::shared_ptr<Scene>  _scene;  ///< The scene to render
	std::unique_ptr<Culler> _culler; ///<Objects culler.

	bool _applySSAO			 = true;  ///< Screen space ambient occlusion.
	ShadowMode  _shadowMode	 = ShadowMode::VARIANCE;  ///< Shadow mapping technique to use.


};
