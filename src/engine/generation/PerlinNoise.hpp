#pragma once

#include "resources/Texture.hpp"
#include "Common.hpp"

/**
 \brief Generate 3D perlin noise and multi-layered noise.
 \ingroup Generation
 */
class PerlinNoise {

public:
	/**
	 Constructor. Initialize the randomness table.
	 */
	PerlinNoise();

	/**
	 Fill all components of an image with Perlin noise in [-1,1].
	 \param image image to fill with preset dimensions
	 \param scale the frequency, in pixels
	 \param offset the origin in sampled noise space
	 */
	void generate(Image & image, float scale, const glm::vec3 & offset = glm::vec3(0.0f));

	/**
	 Fill all components of an image with multi-layered Perlin noise (FBM).
	 \param image image to fill with preset dimensions
	 \param octaves number of layers
	 \param gain the amplitude ratio between a layer and the previous one
	 \param lacunarity the frequency ratio between a layer and the previous one
	 \param scale the base frequency, in pixels
	 \param offset the origin in sampled noise space
	*/
	void generateLayers(Image & image, int octaves, float gain, float lacunarity, float scale, const glm::vec3 & offset = glm::vec3(0.0f));

	/** Regenerate the randomness table with new values. */
	void reseed();

private:

	/** Compute the dot product between a direction vector and the gradient at a (ix, iy, iz) location on the grid.
	 \param ip the grid vertex location
	 \param dp the direction vector
	 \return the dot product
	 */
	float dotGrad(const glm::ivec3 & ip, const glm::vec3 & dp);

	/** Evaluate Perlin noise for a given location in noise space.
	 \param p location
	 \return the noise value in [-1, 1]
	 */
	float perlin(const glm::vec3 & p);

	std::array<int, 512> _hashes; ///< Permutation table.
	Image _directions; ///< random unit sphere directions.
};
