#pragma once

#include "DeferredLight.hpp"

#include "renderers/DebugLightRenderer.hpp"

#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"

#include "resources/Texture.hpp"


#include "Common.hpp"

/**
 \brief Provide debuggging visualization for most scene elements.
 \ingroup PBRDemo
 */
class DebugRenderer final : public Renderer {

public:
	/** Constructor.
	 */
	explicit DebugRenderer();

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** \copydoc Renderer::draw */
	void draw(const Camera & camera, Texture* dstColor, Texture* dstDepth, uint layer = 0) override;

	/** \copydoc Renderer::interface */
	void interface() override;

	~DebugRenderer();

private:

	/** Probe info to visualize. */
	enum class ProbeMode : int {
		SHCOEFFS = 0, ///< Irradiance SH coeffs.
		RADIANCE = 1 ///< (preconvolved) radiance cubemap.
	};

	/** Update the bounding boxes mesh. */
	void updateSceneMesh();

	DebugLightRenderer _lightDebugRenderer; ///< The lights debug renderer.
	std::shared_ptr<Scene> _scene;			///< The scene to render

	const Mesh * _sphere = nullptr;			///< Sphere mesh.
	Program * _probeProgram = nullptr;///< Light probe visu.
	Program * _boxesProgram = nullptr;///< Bounding boxes visu.
	Program * _frameProgram = nullptr;///< Frame and gizmo visu.

	Mesh _sceneBoxes; ///< Bounding boxes of all scene objects.
	Mesh _frame; ///< Gizmo and grid in world space.
	Mesh _cubeLines; ///< General wireframe cube.

	ProbeMode _probeMode = ProbeMode::RADIANCE; ///< Probe info to display.
	float _probeRoughness = 0.0f; ///< Radiance level to display.
	bool _showLights = true; ///< Show lights wireframe.
	bool _showProbe = true; ///< Show probe data.
	bool _showBoxes = true; ///< Show scene objects bounding boxes.
	bool _showFrame = true; ///< Show world frame (grid and gizmo).
};
