#pragma once

#include "renderers/Renderer.hpp"
#include "graphics/Program.hpp"
#include "GameMenu.hpp"

/**
 \brief Renders a game menu.
 \ingroup SnakeGame
 */
class GameMenuRenderer final : public Renderer {
public:
	/** Constructor.
	 */
	explicit GameMenuRenderer();

	/** Draw the menu
	 \param menu the menu to draw
	 \param finalRes the final viewport dimensions
	 \param aspectRatio the target aspect ratio
	 */
	void drawMenu(const GameMenu & menu, const glm::vec2 & finalRes, float aspectRatio) const;

	/** Return the absolute unit size of the button mesh.
	 \return the dimensions of the button mesh
	 */
	glm::vec2 getButtonSize() const;
	
private:
	Program * _backgroundProgram; ///< Background images rendering.
	Program * _buttonProgram;		///< Buttons rendering.
	Program * _imageProgram;		///< Fixed images rendering.
	Program * _fontProgram;		///< Labels font rendering.
	const Mesh * _button;				///< Button main mesh (with border).
	const Mesh * _buttonIn;				///< Button interior mesh.
	const Mesh * _toggle;				///< Toggle main mesh (with border).
	const Mesh * _toggleIn;				///< Toggle interior mesh.
	const Mesh * _quad;					///< Quad mesh for images.
};
