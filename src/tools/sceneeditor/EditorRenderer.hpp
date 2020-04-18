#pragma once
#include "renderers/DebugLightRenderer.hpp"

#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"

#include "Common.hpp"

/**
 \brief Render the scene in the editor.
 \ingroup SceneEditor
 */
class EditorRenderer final : public Renderer {

public:
	/** Constructor.
	 \param resolution the rendering resolution
	 */
	explicit EditorRenderer(const glm::vec2 & resolution);

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** \copydoc Renderer::draw */
	void draw(const Camera & camera) override;

	/** \copydoc Renderer::clean */
	void clean() override;

	/** \copydoc Renderer::resize
	 */
	void resize(unsigned int width, unsigned int height) override;

private:
	
	/** Render background object.
	 \param view the view matrix
	 \param proj the projection matrix
	 \param pos the viewer position
	 */
	void renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos);

	DebugLightRenderer _lightsDebug; ///< Lights wireframe renderer.
	
	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Scene buffer.

	const Program * _objectProgram; ///< Basic object program.
	const Program * _skyboxProgram; ///< Skybox program.
	const Program * _bgProgram;		///< 2D background program.
	const Program * _atmoProgram;	///< Atmosphere shader.
	std::shared_ptr<Scene> _scene;	///< The scene to render.
	
};
