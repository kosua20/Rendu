#pragma once
#include "Common.hpp"

#include "graphics/GLUtilities.hpp"

/**
 \brief Contains font informations: texture atlas, codepoints supported, dimensions of each glyph.
 Assumes that the supported codepoints form a continuous range.
 \ingroup Resources
 */
struct FontInfos {
	
	/** \brief
	 A font glyph bounding box, in UV space.
	 */
	struct Glyph {
		glm::vec2 min; ///< Bottom left corner.
		glm::vec2 max; ///< Top right corner.
	};
	
	TextureInfos * atlas; ///< The font texture atlas.
	int firstCodepoint; ///< The integer value of the first supported character.
	int lastCodepoint; ///< The integer value of the last supported character.
	glm::vec2 margins; ///< Margin to apply around each characters when generating the geometry.
	
	std::vector<Glyph> glyphs; ///<The glyphs informations.
	
	
};

/**
 \brief Font loading and labels generation utilities.
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
	static void loadFont(std::istream & in, FontInfos & font);
	
	/** Generate the label mesh for a given text and font.
	 \param text the text do display
	 \param font the font to use for the label
	 \param scale the vertical height of the characters, in absolute units
	 \param align the text alignment to apply, will influence the origin placement
	 \return the infos of the label mesh
	 */
	static MeshInfos generateLabel(const std::string & text, const FontInfos & font, const float scale, const Alignment align = LEFT );
	
};
