#include "TextUtilities.hpp"
#include "resources/ResourcesManager.hpp"
#include <sstream>


void TextUtilities::loadFont(std::istream & in, FontInfos & font){
	
	std::string line;
	std::vector<std::string> lines;
	while(!in.eof()){
		std::getline(in, line);
		if(line.empty() || line[0] == '#'){
			continue;
		}
		lines.push_back(line);
	}
	// We expect at least 4 lines.
	if(lines.size() < 4){
		Log::Error() << Log::Resources << "Unable to parse font." << std::endl;
		return;
	}
	font.atlas = Resources::manager().getTexture(lines[0], {GL_R8});
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


MeshInfos TextUtilities::generateLabel(const std::string & text, const FontInfos & font, const float scale, const Alignment align){
	Mesh mesh;
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
	if(align != LEFT){
		const float shiftX = (align == CENTER ? 0.5f : 1.0f) * currentOrigin.x;
		for(auto & vert : mesh.positions){
			vert.x -= shiftX;
		}
	}
	return GLUtilities::setupBuffers(mesh);
	
}


std::string TextUtilities::trim(const std::string & str, const std::string & del){
	const size_t firstNotDel = str.find_first_not_of(del);
	if(firstNotDel == std::string::npos){
		return "";
	}
	const size_t lastNotDel = str.find_last_not_of(del);
	return str.substr(firstNotDel, lastNotDel - firstNotDel + 1);
}

std::string TextUtilities::removeExtension(std::string & str){
	const std::string::size_type pos = str.find_last_of(".");
	if(pos == std::string::npos){
		return "";
	}
	const std::string ext(str.substr(pos));
	str.erase(str.begin() + pos, str.end());
	return ext;
}



void TextUtilities::replace(std::string & source, const std::string& fromString, const std::string & toString){
	std::string::size_type nextPos = 0;
	const size_t fromSize = fromString.size();
	const size_t toSize = toString.size();
	while((nextPos = source.find(fromString, nextPos)) != std::string::npos){
		source.replace(nextPos, fromSize, toString);
		nextPos += toSize;
	}
}
