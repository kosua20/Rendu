#include "generation/Random.hpp"

void Random::seed() {
	std::random_device rd;
	_seed = rd();
	Random::seed(_seed);
}

void Random::seed(unsigned int seedValue) {
	_seed = seedValue;
	// Seed the shared MT generator.
	_shared = std::mt19937(_seed);
	// Reset the calling thread generator.
	_thread = LocalMT19937();
}

unsigned int Random::getSeed() {
	return _seed;
}

int Random::Int(int min, int max) {
	return (std::uniform_int_distribution<int>(min, max)(_thread.mt));
}

float Random::Float() {
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(_thread.mt);
}

float Random::Float(float min, float max) {
	return std::uniform_real_distribution<float>(min, max)(_thread.mt);
}


glm::vec3 Random::Color(){
	const float hue = Random::Float(0.0f, 360.0f);
	const float saturation = Random::Float(0.5f, 0.95f);
	const float value = Random::Float(0.5f, 0.95f);
	return glm::rgbColor(glm::vec3(hue, saturation, value));
}

glm::vec2 Random::sampleDisk(){
	const float x = 2.0f * Random::Float() - 1.0f;
	const float y = 2.0f * Random::Float() - 1.0f;
	if(x == 0.0f && y == 0.0f){
		return glm::vec2(0.0f,0.0f);
	}
	float angle, radius;
	if(std::abs(x) > std::abs(y)){
		radius = x;
		angle = glm::quarter_pi<float>() * y / x;
	} else {
		radius = y;
		angle = glm::half_pi<float>() - glm::quarter_pi<float>() * x / y;
	}
	return radius * glm::vec2(std::cos(angle), std::sin(angle));
}

glm::vec3 Random::sampleSphere() {
	const float thetaCos = 2.0f * Random::Float() - 1.0f;
	const float phi		 = glm::two_pi<float>() * Random::Float();
	const float thetaSin = std::sqrt(1.0f - thetaCos * thetaCos);
	return glm::vec3(thetaSin * std::cos(phi), thetaSin * std::sin(phi), thetaCos);
}

glm::vec3 Random::sampleCosineHemisphere(){
	// Sample the disk and project onto the hemisphere.
	const glm::vec2 xy = Random::sampleDisk();
	const float z = std::sqrt(std::max(0.0f, 1.0f - xy.x * xy.x - xy.y * xy.y));
	return glm::vec3(xy.x, xy.y, z);
}

Random::LocalMT19937::LocalMT19937() {
	// Get a lock on the shared MT generator.
	std::lock_guard<std::mutex> guard(_lock);
	// Generate a local seed.
	seed = std::uniform_int_distribution<>()(Random::_shared);
	// Initialize thread MT generator using this seed.
	mt = std::mt19937(seed);
	// Lock is released at end of scope.
}

unsigned int Random::_seed;
std::mt19937 Random::_shared;
std::mutex Random::_lock;
thread_local Random::LocalMT19937 Random::_thread;
