#include "generation/PerlinNoise.hpp"
#include "generation/Random.hpp"
#include "system/System.hpp"

PerlinNoise::PerlinNoise() {
	reseed();
}

void PerlinNoise::generate(Image & image, float scale, const glm::vec3 & offset){
	System::forParallel(0, size_t(image.height), [&image, scale, &offset, this](size_t y){
		for(uint x = 0; x < image.width; ++x){
			for(uint c = 0; c < image.components; ++c){
				const glm::vec3 p = offset + scale * glm::vec3(x,y,c);
				image.rgba(int(x), int(y))[c] = perlin(p);
			}
		}
	});
}

void PerlinNoise::generateLayers(Image & image, int octaves, float gain, float lacunarity, float scale, const glm::vec3 & offset){
	float weight = 1.0f;
	for(int i = 0; i < octaves; ++i){
		Image img(image.width, image.height, image.components);
		generate(img, scale, offset);
		System::forParallel(0, size_t(image.height), [&image, weight, &img](size_t y){
			for(uint x = 0; x < image.width; ++x){
				for(uint c = 0; c < image.components; ++c){
					image.rgba(x, uint(y))[c] += weight * img.rgba(x, uint(y))[c];
				}
			}
		});
		scale *= lacunarity;
		weight *= gain;
	}
}

void PerlinNoise::reseed(){
	// Generate a permutation of 0-255 indices.
	std::vector<int> halfHashes(256);
	for(size_t i = 0; i < halfHashes.size(); ++i){
		halfHashes[i] = int(i);
	}
	Random::shuffle(halfHashes);
	// Pad the shuffled indices to simplify lookup.
	for(size_t i = 0; i < halfHashes.size(); ++i){
		_hashes[i] = halfHashes[i];
		_hashes[i+256] = halfHashes[i];
	}

	// Sample random unit directions on the sphere.
	_directions = Image(64, 64, 3);
	for(uint y = 0; y < _directions.height; ++y){
		for(uint x = 0; x < _directions.width; ++x){
			_directions.rgb(int(x), int(y)) = glm::normalize(Random::sampleSphere());
		}
	}
}

float PerlinNoise::dotGrad(const glm::ivec3 & ip, const glm::vec3 & dp){
	const int id = _hashes[_hashes[_hashes[ip.x] + ip.y] + ip.z];
	const glm::vec3 & grad = _directions.rgb(id/64, id%64);
	return glm::dot(grad, dp);
}

float PerlinNoise::perlin(const glm::vec3 & p){
	const glm::vec3 x = p;
	glm::ivec3 ix = glm::ivec3(glm::floor(x));
	const glm::vec3 dx = x - glm::vec3(ix);
	ix &= (256-1);
	// Fetch cell gradients.
	glm::vec4 g0s, g1s;
	g0s[0] = dotGrad(ix        , dx        );
	g0s[1] = dotGrad(ix + glm::ivec3(0, 1, 0), dx - glm::vec3(0, 1, 0));
	g0s[2] = dotGrad(ix + glm::ivec3(0, 0, 1), dx - glm::vec3(0, 0, 1));
	g0s[3] = dotGrad(ix + glm::ivec3(0, 1, 1), dx - glm::vec3(0, 1, 1));
	g1s[0] = dotGrad(ix + glm::ivec3(1, 0, 0), dx - glm::vec3(1, 0, 0));
	g1s[1] = dotGrad(ix + glm::ivec3(1, 1, 0), dx - glm::vec3(1, 1, 0));
	g1s[2] = dotGrad(ix + glm::ivec3(1, 0, 1), dx - glm::vec3(1, 0, 1));
	g1s[3] = dotGrad(ix + glm::ivec3(1, 1, 1), dx - glm::vec3(1, 1, 1));
	// Compute weights.
	const glm::vec3 dx3 = dx * dx * dx;
	const glm::vec3 weights = ((6.0f * dx - 15.0f) * dx + 10.0f) * dx3;
	// Final value.
	const glm::vec4 gs = glm::mix(g0s, g1s, weights.x);
	const glm::vec2 g = glm::mix(glm::vec2(gs.x, gs.z), glm::vec2(gs.y, gs.w), weights.y);
	return glm::mix(g.x, g.y, weights.z);
}
