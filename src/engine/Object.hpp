#ifndef Object_h
#define Object_h
#include "resources/ResourcesManager.hpp"

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


class Object {

public:

	enum Type {
		Skybox = 0, Regular = 1, Parallax = 2, Custom = 3
	};

	Object();

	~Object();

	/// Init function
	Object(const Object::Type & type, const std::string& meshPath, const std::vector<std::pair<std::string, bool>>& texturesPaths, const std::vector<std::pair<std::string, bool>>& cubemapPaths = {}, bool castShadows = true);
	
	Object(std::shared_ptr<ProgramInfos> & program, const std::string& meshPath, const std::vector<std::pair<std::string, bool>>& texturesPaths, const std::vector<std::pair<std::string, bool>>& cubemapPaths = {});
	
	/// Update function
	void update(const glm::mat4& model);
	
	/// Draw function
	void draw(const glm::mat4& view, const glm::mat4& projection) const;
	
	/// Draw depth function
	void drawDepth(const glm::mat4& lightVP) const;
	
	/// Clean function
	void clean() const;


private:
	
	std::shared_ptr<ProgramInfos> _program;
	std::shared_ptr<ProgramInfos> _programDepth;
	MeshInfos _mesh;
	
	std::vector<TextureInfos> _textures;
	
	glm::mat4 _model;
	
	int _material;
	bool _castShadow;

};

#endif
