#pragma once
#include "EditorRenderer.hpp"
#include "Application.hpp"
#include "input/ControllableCamera.hpp"

#include "Common.hpp"

/**
 \brief Scene editor interface.
 \ingroup SceneEditor
 */
class SceneEditor final : public CameraApp {

public:
	/** Constructor.
	 \param config the configuration to apply
	 */
	explicit SceneEditor(RenderingConfig & config);

	/** Draw the scene and effects */
	void draw() override;

	/** Perform once-per-frame update (buttons, GUI,...) */
	void update() override;
	
	void physics(double fullTime, double frameTime) override;
	
	/** Clean internal resources. */
	void clean() override;

	/** Handle a window resize event.
	 */
	void resize() override;

private:
	
	void setScene(const std::shared_ptr<Scene> & scene);
	
	EditorRenderer _renderer;
	const Program * _passthrough;   ///< Passthrough program.
	
	std::vector<std::shared_ptr<Scene>> _scenes; ///< The existing scenes.
	std::vector<std::string> _sceneNames; ///< The associated scene names.
	unsigned int _currentScene = 0; ///< Currently selected scene.
	float _cameraFOV	 = 70.0f;	///< The adjustable camera fov in degrees.
	bool _paused = false;			///< Pause the scene animations.
};
