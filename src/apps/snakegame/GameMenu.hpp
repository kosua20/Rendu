#pragma once

#include "Common.hpp"
#include "GameMenu.hpp"
#include "resources/Font.hpp"

/**
 \brief Represents a button in a menu.
 \ingroup SnakeGame
 */
struct MenuButton {
	
public:
	
	/** Creates a new momentaneous button.
	 \param screenPos position on screen in absolute units
	 \param meshSize the size of the button mesh
	 \param screenScale scaling to apply to the toggle on the X axis
	 \param actionTag the ID of the action associated to this button
	 \param texture the text texture on the button
	 */
	MenuButton(const glm::vec2 & screenPos, const glm::vec2 & meshSize, const float screenScale, const int actionTag, const Texture * texture);
	
	/** Check if a position is inside the button.
	 \param mousePos the position to test (in absolute units)
	 \return a boolean denoting if the tested position falls inside the button on screen.
	 */
	bool contains(const glm::vec2 & mousePos);
	
	/**
	 \brief Button state.
	 */
	enum State {
		OFF, HOVER, ON
	};
	State state = OFF; ///< The button interaction state.
	
	glm::vec2 pos; ///< Screen position.
	glm::vec2 size; ///< Screen size.
	glm::vec2 scale; ///< Screen scale.
	float displayScale; ///< Initial display scale.
	int tag; ///< Action ID.
	const Texture * tid; ///< Text texture.
};


/**
 \brief Represents a toggle in a menu.
 \ingroup SnakeGame
 */
struct MenuToggle: public MenuButton {
public:
	
	/** Creates a new toggle button which can be either on or off.
	 \param screenPos position on screen in absolute units
	 \param meshSize the size of the checkbox mesh
	 \param screenScale scaling to apply to the toggle on the X axis
	 \param actionTag the ID of the action associated to this button
	 \param texture the text texture accompanying the checkbox
	 */
	MenuToggle(const glm::vec2 & screenPos, const glm::vec2 & meshSize, const float screenScale, const int actionTag, const Texture * texture);
	
	glm::vec2 posBox; ///< Screen position of the toggle box.
	glm::vec2 posImg; ///< Screen position of the text.
	glm::vec2 scaleBox; ///< Scaling of the toggle box.
	const float checkBoxScale = 0.65f; ///< Scaling of checkboxes compared to regular buttons.
};


/**
 \brief Represents a fixed image displayed in a menu.
 \ingroup SnakeGame
 */
struct MenuImage {
public:
	
	/** Creates a menu image.
	 \param screenPos position on screen in absolute units
	 \param screenScale scaling to apply to the image on the X axis
	 \param texture the texture to display
	 */
	MenuImage(const glm::vec2 & screenPos, const float screenScale, const Texture * texture);
	
	glm::vec2 pos; ///< Image position.
	glm::vec2 size; ///< Screen size.
	glm::vec2 scale; ///< Scaling.
	const Texture * tid; ///< Texture.
	
};

/**
 \brief A dynamic text label.
 \ingroup SnakeGame
 */
struct MenuLabel {
public:
	
	/** Creates a label. The position is in the bottom left corner if the alignment is LEFT,
	 the bottom right if the alignment is RIGHT, and in the middle of the label if it is CENTER.
	 \param screenPos position on screen in absolute units
	 \param verticalScale height of the characters in absolute units
	 \param font the font to use
	 \param alignment the text alignment to use
	 */
	MenuLabel(const glm::vec2 & screenPos, const float verticalScale, const Font * font, const Font::Alignment alignment);
	
	/** Update the string displayed by the label.
	 \param text the new text to display
	 */
	void update(const std::string & text);
	
	Mesh mesh; ///< Label mesh.
	glm::vec2 pos; ///< Label position.
	const Texture * tid; ///< Font texture shortcut.
	
private:
	float _vScale; ///< Vertical size on screen.
	const Font * _font; ///< Font atlas.
	Font::Alignment _align; ///< Text alignement.
};


/**
 \brief A game menu containing buttons, toggles and images.
 \ingroup SnakeGame
 */
class GameMenu {
public:
	
	/** Update dimensions of element based on the current window size.
	 \param screenResolution the current window resolution
	 \param initialRatio the window ratio used to describe the initial layout
	 */
	void update(const glm::vec2 & screenResolution, const float initialRatio);
	
	std::vector<MenuButton> buttons; ///< The menu buttons.
	std::vector<MenuToggle> toggles; ///< The menu toggles.
	std::vector<MenuImage> images; ///< The menu images.
	std::vector<MenuLabel> labels; ///< The menu custom labels.
	const Texture * backgroundImage = nullptr; ///< The background texure.
};
