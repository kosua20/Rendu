#include "Player.hpp"
#include "input/Input.hpp"
#include "helpers/GenerationUtilities.hpp"

Player::Player() {	
	_path.resize(_numSamplesPath, {glm::vec2(0.0f), 0.0f } );
}

void Player::update(){
	if(_currentFrame % _samplingPeriod == 0){
		// Check if current head position is different from last recorded.
		const glm::vec2 pos2d(_position);
		const float distance = glm::distance(pos2d, _path[_currentSample].pos);
		if(distance > _minSamplingDistance){
			// Move to the next position in the cyclic buffer.
			_currentSample = (_currentSample + 1) % _numSamplesPath;
			_path[_currentSample].pos = pos2d;
			_path[_currentSample].dist = distance;
		}
	}
	// Increase frame count.
	_currentFrame = (_currentFrame + 1) % _samplingPeriod;
}

void Player::physics(double fullTime, const double frameTime) {
	
	const float deltaSpeed = frameTime * _headAccel;
	const float deltaAngle = frameTime  * _angleSpeed;
	
	bool updatedAngle = false;
	if(Input::manager().pressed(Input::KeyA)){
		_angle += deltaAngle;
		updatedAngle = true;
	}
	if(Input::manager().pressed(Input::KeyD)){
		_angle -= deltaAngle;
		updatedAngle = true;
	}
	if(updatedAngle){
		_momentum[0] = -std::sin(_angle);
		_momentum[1] =  std::cos(_angle);
	}
	
	
	const glm::vec3 translation = float(frameTime) * deltaSpeed * _momentum;
	
	_position += translation;
	if(std::abs(_position[0]) > _maxPos[0]){
		_momentum[0] *= -1.0f;
		_angle = M_PI - _angle + M_PI;
		// Add a few frames of invicibility for acute angles.
		_invicibility += 0.5f;
	}
	if(std::abs(_position[1]) > _maxPos[1]){
		_momentum[1] *= -1.0f;
		_angle = -_angle + M_PI;
		// Add a few frames of invicibility for acute angles.
		_invicibility += _invicibilityIncrease;
	}
	_position = glm::clamp(_position, -_maxPos, _maxPos);
	glm::vec2 headPos(_position);
	
	
	// Are we intersecting any item ?
	for(int i = _items.size() - 1; i >= 0; --i){
		if(glm::distance(_items[i], headPos) < _eatingDistance*_radius){
			// Eat the element.
			_positions.push_back(_items[i]);
			_angles.push_back(0.0f);
			modelsBody.emplace_back(1.0f);
			_items.erase(_items.begin() + i);
			modelsItem.erase(modelsItem.begin() + i);
			_score += _itemValue;
			Log::Info() << "Score: " << _score << "!" << std::endl;
		}
	}
	
	// Animate snake segments.
	if(!_positions.empty()){
		int id = 0;
		float targetDistance = (id+1) * _radius * 2.0f;
		// Initialize with the segment between the head and the current segment.
		glm::vec2 nextPoint = _path[_currentSample].pos;
		glm::vec2 previousPoint = headPos;
		// Add an extra shift to add some space while keeping the head centered for collision tests.
		float newDist = glm::distance(nextPoint, previousPoint) - 0.2f;
		float totalDistance = newDist;
		// Then iterate over each sample segment of the path.
		for(int sid = 0; sid < _numSamplesPath; ++sid){
			while(totalDistance >= targetDistance && (id < _positions.size())){
				const float fraction1 = 1.0f-(totalDistance - targetDistance)/(newDist);
				_positions[id] = glm::mix(previousPoint, nextPoint, fraction1);
				const glm::vec2 dir = glm::normalize((id > 0 ? _positions[id-1] : previousPoint) - nextPoint);
				// \todo Cleanup angle estimation.
				float newAngle = std::atan2(dir[1], dir[0]);
				// Ensure that the angle is not too far from the current one.
				if(std::abs(_angles[id] - newAngle) > std::abs(_angles[id] + newAngle)){
					newAngle *= -1.0f;
				}
				// Blend between the current angle and the target one for a smooth animation.
				_angles[id] = glm::mix(_angles[id], newAngle, frameTime);
				// Bring back into the -pi,pi range to avoid accumulation.
				if(_angles[id] > M_PI){
					_angles[id] -= 2*M_PI;
				}
				if(_angles[id] < -M_PI){
					_angles[id] += 2*M_PI;
				}
				
				++id;
				targetDistance = (id+1) * _radius * 2.0f;
			}
			if(id >= _positions.size()){
				break;
			}
			// Find the previous point.
			int pid = _currentSample - sid;
			if(pid < 0){ pid += _numSamplesPath; }
			previousPoint = _path[pid].pos;
			newDist = _path[pid].dist;
			// Find the next point (the one registered before).
			int nid = pid - 1;
			if(nid < 0){ nid += _numSamplesPath; }
			nextPoint = _path[nid].pos;
			totalDistance += newDist;
		}
		
	}
	
	// Spawn new elements
	if(fullTime > _lastSpawn + _spawnPeriod && _items.size() < _maxItems){
		_lastSpawn = fullTime;
		
		bool found = false;
		glm::vec2 newPos;
		int tests = 0;
		const float maxX = _maxPos[0] - _radius;
		const float maxY = _maxPos[1] - _radius;
		const float minDistance = _minSpawnDistance*_radius;
		do {
			found = true;
			newPos = glm::vec2(Random::Float(-maxX, maxX), Random::Float(-maxY, maxY));
			if(glm::distance(headPos, newPos) < minDistance){
				found = false;
			}
			
			if(found){
				for(const auto & pos : _positions){
					if(glm::distance(pos, newPos) < minDistance){
						found = false;
						break;
					}
				}
			}
			
			if(found){
				for(const auto & pos : _items){
					if(glm::distance(pos, newPos) < minDistance){
						found = false;
						break;
					}
				}
			}
			++tests;
		} while(!found && tests < _spawnTentatives);
		
		if(found){
			_items.push_back(newPos);
			modelsItem.emplace_back(1.0f);
		}
		
	}
	
	// Are we intersecting OURSELVES ?
	bool boom = false;
	if(_invicibility <= 0.0f){
		for(int i = _positions.size() - 1; i >= 0; --i){
			if(glm::distance(_positions[i], headPos) < _collisionDistance*_radius){
				// Noooooo
				boom = true;
				break;
			}
		}
	} else {
		_invicibility = std::max(0.0f, _invicibility - float(frameTime));
	}
	if(boom){
		_alive = false;
	}
	
	
}


void Player::updateModels(){
	
	modelHead = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), _position), _angle, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(_radius));
	
	for(int i = 0; i < _positions.size();++i){
		modelsBody[i] = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(_positions[i], 0.0f)), _angles[i], glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(_radius));
	}
	for(int i = 0; i < _items.size();++i){
		modelsItem[i] = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(_items[i], 0.0f)), glm::vec3(_radius));
	}
}
