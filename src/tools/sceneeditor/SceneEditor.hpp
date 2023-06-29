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
	explicit SceneEditor(RenderingConfig & config, Window & window);

	/** Draw the scene and effects */
	void draw() override;

	/** Perform once-per-frame update (buttons, GUI,...) */
	void update() override;
	
	void physics(double fullTime, double frameTime) override;

	/** Handle a window resize event.
	 */
	void resize() override;

private:

	/** Set the scene to display.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);
	
	EditorRenderer _renderer; 	///< Scene renderer.
	Texture _sceneColor; 		///< Scene texture.
	Texture _sceneDepth; 		///< Scene depth texture.
	Program * _passthrough;  	 ///< Passthrough program.
	
	std::vector<std::shared_ptr<Scene>> _scenes; ///< The existing scenes.
	std::vector<std::string> _sceneNames; ///< The associated scene names.
	int _selectedObject = -1; ///< Currently selected object.
	
	size_t _currentScene = 0; ///< Currently selected scene.
	bool _paused = false;			///< Pause the scene animations.
};
