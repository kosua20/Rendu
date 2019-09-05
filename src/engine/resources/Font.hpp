#pragma once

#include "resources/Texture.hpp"
#include "resources/Mesh.hpp"
#include "Common.hpp"

/**
 \brief Font loading and storage: texture atlas, codepoints supported, dimensions of each glyph.
 Assumes that the supported codepoints form a continuous range.
 \ingroup Resources
 */
class Font {
public:
	/** \brief Text alignment */
	enum class Alignment {
		LEFT,
		CENTER,
		RIGHT
	};

	/** Load font from text stream.
	 \param in the input stream containing the metadata
	 */
	explicit Font(std::istream & in);

	/** Generate the label mesh for a given text and font.
	 \param text the text do display
	 \param scale the vertical height of the characters, in absolute units
	 \param mesh the mesh to populate
	 \param align the text alignment to apply, will influence the origin placement
	 */
	void generateLabel(const std::string & text, float scale, Mesh & mesh, Alignment align = Alignment::LEFT) const;
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Font & operator=(const Font &) = delete;

	/** Copy constructor (disabled). */
	Font(const Font &) = delete;

	/** Move assignment operator.
	 \return a reference to the object assigned to
	 */
	Font & operator=(Font &&) = default;

	/** Move constructor. */
	Font(Font &&) = default;

	/** Obtain the font atlas.
	 \return the font atlas texture.
	 */
	const Texture * atlas() const { return _atlas; }
	
private:
	
	/** \brief
	 A font glyph bounding box, in UV space.
	 */
	struct Glyph {
		glm::vec2 min; ///< Bottom left corner.
		glm::vec2 max; ///< Top right corner.
	};
	
	const Texture * _atlas; ///< The font texture atlas.
	int _firstCodepoint;	///< The integer value of the first supported character.
	int _lastCodepoint;	 ///< The integer value of the last supported character.
	glm::vec2 _margins;	 ///< Margin to apply around each characters when generating the geometry.
	
	std::vector<Glyph> _glyphs; ///<The glyphs informations.
};
