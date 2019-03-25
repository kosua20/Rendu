#ifndef TextUtilities_h
#define TextUtilities_h
#include "Common.hpp"

#include "graphics/GLUtilities.hpp"

struct FontInfos {
	TextureInfos atlas;
	int firstCodepoint;
	int lastCodepoint;
	glm::vec2 margins;
	
	struct Glyph {
		glm::vec2 min;
		glm::vec2 max;
	};
	std::vector<Glyph> glyphs;
};

/**
 \brief Provides utilities to load fonts and generate labels.
 \ingroup Resources
 */
class TextUtilities {

public:
	
	/** \brief Text alignment */
	enum Alignment {
		LEFT, CENTER, RIGHT
	};
	
	static void loadFont(std::istream & in, FontInfos & font);
	
	static MeshInfos generateLabel(const std::string & text, const FontInfos & font, const float scale, const Alignment align = LEFT );
	
};

#endif 
