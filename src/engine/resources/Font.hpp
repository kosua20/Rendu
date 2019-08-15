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
	enum Alignment {
		LEFT, CENTER, RIGHT
	};
	
	/** Load font from text stream.
	 \param in the input stream containing the metadata
	 \param font will be populated with the font metadata and atlas
	 */
	static void loadFont(std::istream & in, Font & font);
	
	/** Generate the label mesh for a given text and font.
	 \param text the text do display
	 \param font the font to use for the label
	 \param scale the vertical height of the characters, in absolute units
	 \param mesh the mesh to populate
	 \param align the text alignment to apply, will influence the origin placement
	 */
	static void generateLabel(const std::string & text, const Font & font, const float scale, Mesh & mesh, const Alignment align = LEFT );
	
	/** \brief
	 A font glyph bounding box, in UV space.
	 */
	struct Glyph {
		glm::vec2 min; ///< Bottom left corner.
		glm::vec2 max; ///< Top right corner.
	};
	
	const Texture * atlas; ///< The font texture atlas.
	int firstCodepoint; ///< The integer value of the first supported character.
	int lastCodepoint; ///< The integer value of the last supported character.
	glm::vec2 margins; ///< Margin to apply around each characters when generating the geometry.
	
	std::vector<Glyph> glyphs; ///<The glyphs informations.
	
};
