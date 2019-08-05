#include "Random.hpp"
#include <iostream>

void Random::seed(){
	std::random_device rd;
	_seed = rd();
	Random::seed(_seed);
}

void Random::seed(unsigned int seedValue){
	_seed = seedValue;
	// Seed the shared MT generator.
	_shared = std::mt19937(_seed);
	// Reset the calling thread generator.
	_thread = LocalMT19937();
}

unsigned int Random::getSeed(){
	return _seed;
}

int Random::Int(int min, int max){
	return (std::uniform_int_distribution<int>(min,max)(_thread.mt));
}

float Random::Float(){
	return std::uniform_real_distribution<float>(0.0f,1.0f)(_thread.mt);
}

float Random::Float(float min, float max){
	return std::uniform_real_distribution<float>(min,max)(_thread.mt);
}

glm::vec3 Random::sampleSphere(){
	const float thetaCos = 2.0f * Random::Float() - 1.0f;
	const float phi = 2.0f * float(M_PI) * Random::Float();
	const float thetaSin = std::sqrt(1.0f - thetaCos * thetaCos);
	return glm::vec3(thetaSin * std::cos(phi), thetaSin * std::sin(phi), thetaCos);	
}

Random::LocalMT19937::LocalMT19937(){
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
