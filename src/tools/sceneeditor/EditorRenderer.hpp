#pragma once
#include "renderers/DebugLightRenderer.hpp"

#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "resources/Texture.hpp"
#include "input/ControllableCamera.hpp"

#include "Common.hpp"

/**
 \brief Render the scene in the editor.
 \ingroup SceneEditor
 */
class EditorRenderer final : public Renderer {

public:
	/** Constructor.
	 */
	explicit EditorRenderer();

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** \copydoc Renderer::draw */
	void draw(const Camera & camera, Texture* dstColor, Texture* dstDepth, uint layer = 0) override;

private:
	
	/** Render background object.
	 \param view the view matrix
	 \param proj the projection matrix
	 \param pos the viewer position
	 */
	void renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos);

	DebugLightRenderer _lightsDebug; ///< Lights wireframe renderer.

	Program * _objectProgram; ///< Basic object program.
	Program * _skyboxProgram; ///< Skybox program.
	Program * _bgProgram;		///< 2D background program.
	Program * _atmoProgram;	///< Atmosphere shader.
	std::shared_ptr<Scene> _scene;	///< The scene to render.
	
};
