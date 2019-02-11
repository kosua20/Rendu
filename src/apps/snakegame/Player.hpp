#ifndef Player_h
#define Player_h
#include "Common.hpp"
#include "resources/ResourcesManager.hpp"

class Player {
public:
	Player();
	
	void update();
	
	void physics(double fullTime, double frameTime);
	
	void draw(const glm::mat4& view, const glm::mat4& projection) ;
	
	void clean() const;
	
	void resize(const glm::vec2 & newRes);
	
private:
	std::shared_ptr<ProgramInfos> _headProgram;
	MeshInfos _head;
	MeshInfos _bodyElement;
	
	glm::mat4 _model = glm::mat4(1.0f);
	
	glm::vec3 _momentum = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 _position = glm::vec3(0.0f);
	float _angle = 0.0f;
	
	std::vector<glm::vec3> _positions;
	std::vector<glm::vec3> _items;
	
	struct PathPoint {
		glm::vec2 pos;
		float dist;
	};
	std::vector<PathPoint> _path;
	unsigned int _currentSample = 0;
	unsigned int _currentFrame = 0;
	glm::vec3 _maxPos = glm::vec3(8.6f, 5.0f, 0.0f);
	double _lastSpawn = 0.0;
	
	const size_t _numSamplesPath = 512;
	const size_t _samplingPeriod = 15;
	const float _radius = 0.5f;
	const float _headAccel = 400.0f;
	const float _angleSpeed = 6.0f;
	
	
};

#endif
