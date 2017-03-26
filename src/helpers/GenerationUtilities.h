#ifndef GenerationUtilities_h
#define GenerationUtilities_h

#include <random>

class Random {
public:
	
	static void seed();
	
	static void seed(double seedValue);
	
	static int Int(int min, int max);
	
	static float Float();
	
	static float Float(float min, float max);
	
	static double getSeed();
	
private:
	
	static double _seed;
	static std::mt19937 _mt;
	static std::uniform_real_distribution<float> _realDist;
};

#endif
