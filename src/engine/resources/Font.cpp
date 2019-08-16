#include "resources/Font.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/TextUtilities.hpp"


void Font::loadFont(std::istream & in, Font & font){
	
	std::string line;
	std::vector<std::string> lines;
	while(!in.eof()){
		std::getline(in, line);
		if(line.empty() || line[0] == '#'){
			continue;
		}
		// Filter potential carriage returns.
		line = TextUtilities::trim(line, "\r");
		lines.push_back(line);
	}
	// We expect at least 4 lines.
	if(lines.size() < 4){
		Log::Error() << Log::Resources << "Unable to parse font." << std::endl;
		return;
	}
	font.atlas = Resources::manager().getTexture(lines[0], {Layout::R8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);
	const glm::vec2 textureSize(font.atlas->width, font.atlas->height);
	
	font.firstCodepoint = int(lines[1][0]);
	font.lastCodepoint = int(lines[2][0]);
	// Compute expected number of glyphs.
	const int expectedCount = font.lastCodepoint - font.firstCodepoint + 1;
	if(int(lines.size()) < 4 + expectedCount){
		Log::Error() << Log::Resources << "Unable to parse glyphs." << std::endl;
		return;
	}
	// Parse margins.
	const std::string::size_type splitPos = lines[3].find(" ");
	font.margins.x = std::stof(lines[3].substr(0, splitPos));
	font.margins.y = std::stof(lines[3].substr(splitPos+1));
	
	// Parse glyphes.
	font.glyphs.resize(expectedCount);
	for(int i = 0; i < expectedCount; ++i){
		// Split in a vec4.
		std::stringstream glyphStr(lines[4+i]);
		int minx, miny, maxx, maxy;
		glyphStr >> minx >> miny >> maxx >> maxy;
		font.glyphs[i].min = (glm::vec2(minx, miny) - font.margins) / textureSize;
		font.glyphs[i].max = (glm::vec2(maxx, maxy) + font.margins) / textureSize;
	}
}

void Font::generateLabel(const std::string & text, const Font & font, const float scale, Mesh & mesh, const Alignment align){
	mesh.clean();
	glm::vec3 currentOrigin(0.0f);
	
	int idBase = 0;
	for(const auto & c : text){
		if(int(c) < font.firstCodepoint || int(c) > font.lastCodepoint){
			Log::Error() << "Unknown codepoint." << std::endl;
			continue;
		}
		const int glyphIndex = int(c) - font.firstCodepoint;
			
		// Indices are easy.
		mesh.indices.push_back(idBase);
		mesh.indices.push_back(idBase+1);
		mesh.indices.push_back(idBase+2);
		mesh.indices.push_back(idBase);
		mesh.indices.push_back(idBase+2);
		mesh.indices.push_back(idBase+3);
		idBase += 4;
		
		const auto & glyph = font.glyphs[glyphIndex];
		// Uvs also.
		mesh.texcoords.push_back(glyph.min);
		mesh.texcoords.push_back(glm::vec2(glyph.max.x, glyph.min.y));
		mesh.texcoords.push_back(glyph.max);
		mesh.texcoords.push_back(glm::vec2(glyph.min.x, glyph.max.y));
		
		// Vertices.
		// We want the vertical height to be scale, X to follow based on aspect ratio in font atlas.
		const glm::vec2 uvSize = (glyph.max - glyph.min);
		float deltaY = scale;
		float deltaX = deltaY * (uvSize.x / uvSize.y) * (font.atlas->width / float(font.atlas->height));
		mesh.positions.push_back(currentOrigin);
		mesh.positions.push_back(currentOrigin + glm::vec3(deltaX, 0.0f, 0.0f));
		mesh.positions.push_back(currentOrigin + glm::vec3(deltaX, deltaY, 0.0f));
		mesh.positions.push_back(currentOrigin + glm::vec3(0.0f, deltaY, 0.0f));
		currentOrigin.x += deltaX;
	}
	// currentOrigin.x now contains the width of the label.
	// Depending on the alignment mode, we shift all vertices based on it.
	if(align != Alignment::LEFT){
		const float shiftX = (align == Alignment::CENTER ? 0.5f : 1.0f) * currentOrigin.x;
		for(auto & vert : mesh.positions){
			vert.x -= shiftX;
		}
	}
	mesh.upload();
	// Remove uneeded CPU geometry.
	mesh.clearGeometry();

}
