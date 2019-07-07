#include "Random.hpp"


void Random::seed(){
	std::random_device rd;
	_seed = rd();
	Random::seed(_seed);
}

void Random::seed(unsigned int seedValue){
	_seed = seedValue;
	_mt = std::mt19937(_seed);
	_realDist = std::uniform_real_distribution<float>(0,1);
}

unsigned int Random::getSeed(){
	return _seed;
}

int Random::Int(int min, int max){
	return (int)(floor(Float() * (max+1 - min)) + min);
}

float Random::Float(){
	return _realDist(_mt);
}

float Random::Float(float min, float max){
	return _realDist(_mt)*(max-min)+min;
}

glm::vec3 Random::sampleSphere(){
	const float thetaCos = 2.0f * Random::Float() - 1.0f;
	const float phi = 2.0f * float(M_PI) * Random::Float();
	const float thetaSin = std::sqrt(1.0f - thetaCos * thetaCos);
	return glm::vec3(thetaSin * std::cos(phi), thetaSin * std::sin(phi), thetaCos);	
}


std::mt19937 Random::_mt;
std::uniform_real_distribution<float> Random::_realDist;
unsigned int Random::_seed;

