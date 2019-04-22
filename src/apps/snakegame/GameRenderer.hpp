#pragma once

#include "renderers/Renderer.hpp"
#include "input/Camera.hpp"
#include "Player.hpp"
#include "processing/SSAO.hpp"


/**
 \brief Renders the main game scene.
 \ingroup SnakeGame
 */
class GameRenderer: public Renderer {
public:
	
	/** Constructor
	 \param config the shared game config
	 */
	GameRenderer(RenderingConfig & config);
	
	/** Draw the game scene
	 \param player the state of the game and player
	 */
	void draw(const Player & player);
	
	/** Perform once-per-frame update (buttons, GUI,...) */
	void update();
	
	/** Resize internal buffers based on new window size.
	 \param width new width
	 \param height new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	/** Clean up rendering resources.*/
	void clean() const;
	
	/** Empty draw */
	void draw(){};
	
	/** Perform physics simulation update (none here).
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 */
	void physics(double fullTime, double frameTime){};
	
	/** Texture ID of the final rendered game.
	 \return the texture ID
	 */
	GLuint finalImage() const ;
	
	/** Current rendering resolution.
	 \return the internal resolution
	 */
	glm::vec2 renderingResolution() const;
	
private:
	
	/** Draw the scene to the current bound framebuffer.
	 \param player the player state
	 */
	void drawScene(const Player & player);
	
	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Scene framebuffer.
	std::unique_ptr<Framebuffer> _lightingFramebuffer; ///< Framebuffer containing the lit result.
	std::unique_ptr<Framebuffer> _fxaaFramebuffer; ///< Framebuffer for postprocess.
	std::unique_ptr<SSAO> _ssaoPass; ///< Screen space ambient occlusion pass.
	
	const ProgramInfos * _fxaaProgram; ///< Antialiasing program.
	const ProgramInfos * _finalProgram; ///< Final upscaling program.
	const ProgramInfos * _coloredProgram; ///< Base scene rendering program.
	const ProgramInfos * _compositingProgram; ///< Lighting program.
	
	const MeshInfos * _ground; ///< Terrain mesh.
	const MeshInfos * _head; ///< Snake head mesh.
	const MeshInfos * _bodyElement; ///< Body elements and items mesh.
	
	Camera _playerCamera; ///< The player camera (fixed).
	const TextureInfos * _cubemap; ///< Environment map for reflections.
};


