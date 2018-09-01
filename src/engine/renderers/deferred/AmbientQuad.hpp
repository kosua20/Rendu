#ifndef AmbientQuad_h
#define AmbientQuad_h
#include "../../Common.hpp"
#include "../../ScreenQuad.hpp"
#include <map>

class AmbientQuad {

public:

	AmbientQuad();

	~AmbientQuad();
	
	void init(std::vector<GLuint> textureIds);
	
	void setSceneParameters(const GLuint reflectionMap, const std::vector<glm::vec3> & irradiance);
	
	/// Draw function,
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void drawSSAO(const glm::mat4& projectionMatrix) const;
		
	void clean() const;
	
private:
	
	GLuint setupSSAO();
	
	std::shared_ptr<ProgramInfos> _program;
	std::shared_ptr<ProgramInfos> _programSSAO;

	std::vector<GLuint> _textures;
	GLuint _textureEnv;
	GLuint _textureBrdf;
	std::vector<GLuint> _texturesSSAO;
	std::vector<glm::vec3> _samples;
	
};

#endif
