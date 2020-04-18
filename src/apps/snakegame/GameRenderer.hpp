#pragma once

#include "renderers/Renderer.hpp"
#include "input/Camera.hpp"
#include "Player.hpp"
#include "processing/SSAO.hpp"

/**
 \brief Renders the main game scene.
 \ingroup SnakeGame
 */
class GameRenderer final : public Renderer {
public:
	/** Constructor
	 \param resolution the rendering resolution
	 */
	explicit GameRenderer(const glm::vec2 & resolution);

	/** Draw the game scene
	 \param player the state of the game and player
	 \param framebuffer the destination framebuffer
	 */
	void drawPlayer(const Player & player, Framebuffer & framebuffer) const;

	/** Resize internal buffers based on new window size.
	 \param width new width
	 \param height new height
	 */
	void resize(unsigned int width, unsigned int height) override;

	/** Clean up rendering resources.*/
	void clean() override;
	
private:
	/** Draw the scene to the current bound framebuffer.
	 \param player the player state
	 */
	void drawScene(const Player & player) const;

	std::unique_ptr<Framebuffer> _sceneFramebuffer;	///< Scene framebuffer.
	std::unique_ptr<Framebuffer> _lightingFramebuffer; ///< Framebuffer containing the lit result.
	std::unique_ptr<SSAO> _ssaoPass;				   ///< Screen space ambient occlusion pass.

	const Program * _fxaaProgram;		 ///< Antialiasing program.
	const Program * _coloredProgram;	 ///< Base scene rendering program.
	const Program * _compositingProgram; ///< Lighting program.

	const Mesh * _ground;	  ///< Terrain mesh.
	const Mesh * _head;		   ///< Snake head mesh.
	const Mesh * _bodyElement; ///< Body elements and items mesh.

	Camera _playerCamera;	 ///< The player camera (fixed).
	const Texture * _cubemap; ///< Environment map for reflections.
};
