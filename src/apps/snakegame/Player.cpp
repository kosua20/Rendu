#include "Player.hpp"
#include "input/Input.hpp"
#include "helpers/GenerationUtilities.hpp"

Player::Player() {
	
	_headProgram = Resources::manager().getProgram("object_basic");
	_head = Resources::manager().getMesh("head");
	_bodyElement = Resources::manager().getMesh("body");
	checkGLError();
	
	_path.resize(_numSamplesPath, {glm::vec2(0.0f), 0.0f } );
}

void Player::update(){
	if(_currentFrame % _samplingPeriod == 0){
		// Check if current head position is different from last recorded.
		const glm::vec2 pos2d(_position);
		const float distance = glm::distance(pos2d, _path[_currentSample].pos);
		if(distance > 0.02f){
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
	_position = glm::clamp(_position, -_maxPos, _maxPos);
	glm::vec2 headPos(_position);
	
	
	// Are we intersecting any item ?
	for(int i = _items.size() - 1; i >= 0; --i){
		if(glm::distance(_items[i], headPos) < 1.5f*_radius){
			// Eat the element.
			_positions.push_back(_items[i]);
			_angles.push_back(0.0f);
			_items.erase(_items.begin() + i);
		}
	}
	
	
	
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
				_angles[id] = std::atan2(dir[1], dir[0]);
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
	const double spawnPeriod = 1.5;
	if(fullTime > _lastSpawn + spawnPeriod && _items.size() < 20){
		_lastSpawn = fullTime;
		
		bool found = false;
		glm::vec2 newPos;
		int tests = 0;
		const float maxX = _maxPos[0] - _radius;
		const float maxY = _maxPos[1] - _radius;
		do {
			found = true;
			newPos = glm::vec2(Random::Float(-maxX, maxX), Random::Float(-maxY, maxY));
			if(glm::distance(headPos, newPos) < 3.0*_radius){
				found = false;
			}
			
			if(found){
				for(const auto & pos : _positions){
					if(glm::distance(pos, newPos) < 3.0*_radius){
						found = false;
						break;
					}
				}
			}
			
			if(found){
				for(const auto & pos : _items){
					if(glm::distance(pos, newPos) < 3.0*_radius){
						found = false;
						break;
					}
				}
			}
			++tests;
		} while(!found && tests < 50);
		
		if(found){
			_items.push_back(newPos);
		}
		
	}
	
	// Are we intersecting OURSELVES ?
	bool boom = false;
	for(int i = _positions.size() - 1; i >= 0; --i){
		if(glm::distance(_positions[i], headPos) < 1.5f*_radius){
			// Noooooo
			boom = true;
			break;
		}
	}
	if(boom){
		Log::Info() << "BOOOOOM I'm dead :(" << std::endl;
	}
	
	
}
void Player::resize(const glm::vec2 & newRes){
}



void Player::draw(const glm::mat4& view, const glm::mat4& projection)  {
	
	// Combine the three matrices.
	// \todo Update model lazily.
	_model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), _position), _angle, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(_radius));
	const glm::mat4 VP = projection * view;

	const glm::mat4 MVP = VP * _model;
	
	// Compute the normal matrix
	//glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	// Select the program (and shaders).
	glUseProgram(_headProgram->id());
	
	// Upload the MVP matrix.
	glUniformMatrix4fv(_headProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
	glBindVertexArray(_head.vId);
	glDrawElements(GL_TRIANGLES, _head.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(_bodyElement.vId);
	for(int i = 0; i < _positions.size();++i){
		_model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(_positions[i], 0.0f)), _angles[i], glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(_radius));
		const glm::mat4 MVP1 = VP * _model;
		glUniformMatrix4fv(_headProgram->uniform("mvp"), 1, GL_FALSE, &MVP1[0][0]);
		glDrawElements(GL_TRIANGLES, _bodyElement.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	glBindVertexArray(_bodyElement.vId);
	for(int i = 0; i < _items.size();++i){
		_model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(_items[i], 0.0f)), glm::vec3(_radius));
		const glm::mat4 MVP1 = VP * _model;
		glUniformMatrix4fv(_headProgram->uniform("mvp"), 1, GL_FALSE, &MVP1[0][0]);
		glDrawElements(GL_TRIANGLES, _bodyElement.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	glBindVertexArray(0);
	glUseProgram(0);
}


void Player::clean() const {
	glDeleteVertexArrays(1, &_head.vId);
	glDeleteVertexArrays(1, &_bodyElement.vId);

}
