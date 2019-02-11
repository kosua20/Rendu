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
	glm::vec2 pos2d(_position);
	
	
	if(!_positions.empty()){
		int id = 0;
		float targetDistance = (id+1) * _radius * 2.0f;
		// Initialize with the segment between the head and the current segment.
		glm::vec2 nextPoint = _path[_currentSample].pos;
		glm::vec2 previousPoint = pos2d;
		// Add an extra shift to add some space while keeping the head centered for collision tests.
		float newDist = glm::distance(nextPoint, previousPoint) - 0.2f;
		float totalDistance = newDist;
		// Then iterate over each sample segment of the path.
		for(int sid = 0; sid < _numSamplesPath; ++sid){
			while(totalDistance >= targetDistance && (id < _positions.size())){
				const float fraction1 = 1.0f-(totalDistance - targetDistance)/(newDist);
				const glm::vec2 newPos1 = glm::mix(previousPoint, nextPoint, fraction1);
				_positions[id] = glm::vec3(newPos1, 0.0f);
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
		glm::vec3 newPos;
		int tests = 0;
		do {
			found = true;
			newPos = glm::vec3(Random::Float(-_maxPos[0], _maxPos[0]), Random::Float(-_maxPos[1], _maxPos[1]), 0.0f);
			if(glm::distance(_position, newPos) < 3.0*_radius){
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
}
void Player::resize(const glm::vec2 & newRes){
	//_maxPos[0] = _maxPos[1]*newRes[0]/std::max(1.0f, newRes[1]);
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
		_model = glm::scale(glm::translate(glm::mat4(1.0f), _positions[i]), glm::vec3(_radius));
		const glm::mat4 MVP1 = VP * _model;
		glUniformMatrix4fv(_headProgram->uniform("mvp"), 1, GL_FALSE, &MVP1[0][0]);
		glDrawElements(GL_TRIANGLES, _bodyElement.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	glBindVertexArray(_bodyElement.vId);
	for(int i = 0; i < _items.size();++i){
		_model = glm::scale(glm::translate(glm::mat4(1.0f), _items[i]), glm::vec3(_radius));
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
//	for (auto & texture : _textures) {
//		glDeleteTextures(1, &(texture.id));
//	}
}
//
//BoundingBox Player::getBoundingBoxWorld() const {
//	return _mesh.bbox.transformed(_model);
//}
//
//BoundingBox Player::getBoundingBoxModel() const {
//	return _mesh.bbox;
//}
