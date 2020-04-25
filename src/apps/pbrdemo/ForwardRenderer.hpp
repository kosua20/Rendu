#pragma once

#include "ForwardLight.hpp"

#include "renderers/DebugLightRenderer.hpp"

#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"

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
	 */
	explicit ForwardRenderer(const glm::vec2 & resolution, ShadowMode mode, bool ssao);

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** \copydoc Renderer::draw */
	void draw(const Camera & camera, Framebuffer & framebuffer, size_t layer = 0) override;

	/** \copydoc Renderer::clean */
	void clean() override;

	/** \copydoc Renderer::resize
	 */
	void resize(unsigned int width, unsigned int height) override;

	/** \copydoc Renderer::interface */
	void interface() override;

	/** \return the framebuffer containing the scene depth information */
	const Framebuffer & depthFramebuffer() const {
		return *_sceneFramebuffer;
	}
	
private:

	/** Render the scene objects to the G-buffer.
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 \param pos the camera position
	 */
	void renderScene(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos);
	
	/** Render the scene background to the G-buffer.
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 \param pos the camera position
	 */
	void renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos);

	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Scene framebuffer
	std::unique_ptr<SSAO> _ssaoPass;				///< SSAO processing.
	std::unique_ptr<ForwardLight> _lightsGPU;	///< The lights renderer.
	
	Program * _objectProgram;		 ///< Basic PBR program
	Program * _objectNoUVsProgram; ///< Basic PBR program
	Program * _parallaxProgram;	 ///< Parallax mapping PBR program
	Program * _emissiveProgram;	 ///< Parallax mapping PBR program
	const Program * _compProgram;   ///< Final ambient qnd direct compositing.

	const Program * _skyboxProgram; ///< Skybox program.
	const Program * _bgProgram;		///< Planar background program.
	const Program * _atmoProgram;   ///< Atmospheric scattering program.

	const Texture * _textureBrdf; ///< The BRDF lookup table.

	std::shared_ptr<Scene> _scene; ///< The scene to render

	bool _applySSAO			 = true;  ///< Screen space ambient occlusion.
	ShadowMode  _shadowMode	 = ShadowMode::VARIANCE;  ///< Shadow mapping technique to use.

	glm::mat4 _frustumMat = glm::mat4(1.0f); ///< View projection matrix backup.
	bool _freezeFrustum = false;			 ///< Freeze the frustum used for culling.
};
