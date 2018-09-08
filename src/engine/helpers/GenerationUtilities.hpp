#ifndef GenerationUtilities_h
#define GenerationUtilities_h

#include <random>

/**
 \brief Generate seedable random numbers of various types and in multiple intervals.
 \ingroup Helpers
 */
class Random {
public:
	
	/** Seed using a random number.
	 \note The seed is obtained through a std::random_device.
	 */
	static void seed();
	
	/** Seed using a given number.
	 \param seedValue the seed to use
	 */
	static void seed(unsigned int seedValue);
	
	/** Generate an integer in a given interval.
	 \param min the included lower bound
	 \param max the included higher bound
	 \return an integer in [min, max]
	 */
	static int Int(int min, int max);
	
	/** Generate a float in [0.0, 1.0)
	 \return a float in [0.0, 1.0)
	 */
	static float Float();
	
	/** Generate a float in a given interval.
	 \param min the included lower bound
	 \param max the excluded higher bound
	 \return a float in [min, max)
	 */
	static float Float(float min, float max);
	
	/** Query the current seed.
	 \return the current seed
	 */
	static unsigned int getSeed();
	
private:
	
	static unsigned int _seed; ///< The current seed.
	static std::mt19937 _mt; ///< The randomness generator.
	static std::uniform_real_distribution<float> _realDist; ///< The unit uniform float distribution used.
};

#endif
