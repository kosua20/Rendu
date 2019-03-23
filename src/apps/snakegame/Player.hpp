#pragma once

#include "Common.hpp"
#include "resources/ResourcesManager.hpp"

/** \brief Player state and animations.
 \ingroup SnakeGame
 */
class Player {
public:
	/** Constructor */
	Player();
	
	/** Update logical state, once per frame */
	void update();
	
	/** Update the physic state of the player snake and items
	 \param fullTime time elapsed since the beginning of the game
	 \param frameTime delta time since last physics update
	 */
	void physics(double fullTime, double frameTime);
	
	/** Update the model matrices of all items
	 This is so that we avoid updating them at each physics step.
	 */
	void updateModels();
	
	/// Status of the player.
	const bool alive(){ return _alive; }
	
	/// Score of the player.
	const int score(){ return _score; }
	
	glm::mat4 modelHead; ///< The snake head model matrix.
	std::vector<glm::mat4> modelsBody; ///< The snake body elements model matrices.
	std::vector<glm::mat4> modelsItem; ///< The edible items model matrices.
	std::vector<int> looksBody; ///< The snake body elements material IDs.
	std::vector<int> looksItem; ///< The edible items material IDs.
	
private:
	
	/** \brief A sample along the snake path.
	 */
	struct PathPoint {
		glm::vec2 pos; ///< Sample position on screen.
		float dist; ///< Distance to the previous sample.
	};
	
	/// The snake momentum.
	glm::vec3 _momentum = glm::vec3(0.0f, 1.0f, 0.0f);
	/// The snake position.
	glm::vec3 _position = glm::vec3(0.0f);
	/// The snake head orientation.
	float _angle = 0.0f;
	/// The snake body elements positions.
	std::vector<glm::vec2> _positions;
	/// The snake body elements orientations.
	std::vector<float> _angles;
	/// The items positions.
	std::vector<glm::vec2> _items;
	/// Ring buffer containing samples along the snake path.
	std::vector<PathPoint> _path;
	/// The current sample in the ring buffer.
	unsigned int _currentSample = 0;
	/// Current frame (modulo sampling period).
	unsigned int _currentFrame = 0;
	/// Time since the last item spawn.
	double _lastSpawn = 0.0;
	/// Invicibility time buffer after boucning off a wall.
	float _invicibility = 0.0f;
	/// Player score.
	int _score = 0;
	/// Player status.
	bool _alive = true;
	
	// Constants.
	/// Terrain bounding box.
	const glm::vec3 _maxPos = glm::vec3(8.6f, 5.0f, 0.0f);
	/// Time betweeen two item spawns.
	const double _spawnPeriod = 1.5;
	/// Size of the samples ring buffer.
	const size_t _numSamplesPath = 512;
	/// Frame count between two samples.
	const size_t _samplingPeriod = 15;
	/// Items and elements radius.
	const float _radius = 0.5f;
	/// Head speed.
	const float _headAccel = 4.0f;
	/// Head angular speed.
	const float _angleSpeed = 6.0f;
	/// Minimum distance between two samples.
	const float _minSamplingDistance = 0.02f;
	/// Amount of time added to invicibility at each bounce.
	const float _invicibilityIncrease = 0.5f;
	/// Distance below which an item can be eaten.
	const float _eatingDistance = 1.5f;
	/// Minimum distance with the snake head when spawning a new item.
	const float _minSpawnDistance = 3.0f;
	/// Distance below which a collision is registered.
	const float _collisionDistance = 1.5f;
	/// Ho many spawn attempts should be made at each spawn event.
	const int _spawnTentatives = 50;
	/// Max number of items on the terrain.
	const int _maxItems = 20;
	/// Score gained when eating an item.
	const int _itemValue = 1;
};


