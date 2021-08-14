#pragma once

#include "resources/Texture.hpp"
#include "Common.hpp"
#include <array>
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
	 Fill a component of an image with Perlin noise in [-1,1].
	 \param image image to fill with preset dimensions
	 \param channel the channel of the image to fill
	 \param scale the frequency, in pixels
	 \param z the depth (in pixels) at which the 3D Perlin noise should be sampled
	 \param offset the origin in sampled noise space
	 */
	void generate(Image & image, uint channel, float scale, float z, const glm::vec3 & offset = glm::vec3(0.0f));


	/**
	 Fill a component of an image with tiling Perlin noise in [-1,1].
	 \param image image to fill with preset dimensions
	 \param channel the channel of the image to fill
	 \param scale the frequency, in pixels
	 \param z the depth (in pixels) at which the 3D Perlin noise should be sampled
	 \param offset the origin in sampled noise space
	 \note The scale might internally be adjusted to ensure periodicity.
	 */
	void generatePeriodic(Image & image, uint channel, float scale, float z, const glm::vec3 & offset = glm::vec3(0.0f));

	/**
	 Fill a component of an image with multi-layered Perlin noise (FBM).
	 \param image image to fill with preset dimensions
	 \param channel the channel of the image to fill	 
	 \param octaves number of layers
	 \param gain the amplitude ratio between a layer and the previous one
	 \param lacunarity the frequency ratio between a layer and the previous one
	 \param scale the base frequency, in pixels
	 \param offset the origin in sampled noise space
	*/
	void generateLayers(Image & image, uint channel, int octaves, float gain, float lacunarity, float scale, const glm::vec3 & offset = glm::vec3(0.0f));

	/** Regenerate the randomness table with new values. */
	void reseed();

private:

	static const int kHashTableSize = 256; ///< Number of hashes used for the generation, limit the periodicity.

	/** Compute the dot product between a direction vector and the gradient at a (ix, iy, iz) location on the grid.
	 \param ip the grid vertex location
	 \param dp the direction vector
	 \return the dot product
	 */
	float dotGrad(const glm::ivec3 & ip, const glm::vec3 & dp);

	/** Evaluate Perlin noise for a given location in noise space.
	 \param p location
	 \param w the tiling period to apply on each axis
	 \return the noise value in [-1, 1]
	 */
	float perlin(const glm::vec3 & p, const glm::ivec3 & w = glm::ivec3(kHashTableSize-1));

	std::array<int, 2*kHashTableSize> _hashes; ///< Permutation table.
	Image _directions; ///< random unit sphere directions.
};
