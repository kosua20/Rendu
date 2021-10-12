#include "Probe.hpp"
#include "graphics/GPUTypes.hpp"
#include "graphics/GPU.hpp"
#include "resources/Library.hpp"
#include "scene/LightProbe.hpp"

Probe::Probe(LightProbe & probe, std::shared_ptr<Renderer> renderer, uint size, uint mips, const glm::vec2 & clippingPlanes) {
	_renderer	 = renderer;
	_framebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(TextureShape::Cube, size, size, 6, mips, {Layout::RGBA16F}, "Probe"));
	_framebuffer->clear(glm::vec4(0.0f), 1.0f);
	_position	 = probe.position();
	_radianceCompute = Resources::manager().getProgramCompute("radiance_convo");
	// Texture used to compute irradiance spherical harmonics.
	_copy = _renderer->createOutput(TextureShape::Cube, 16, 16, 6, 1, "Probe copy");
	_irradianceCompute = Resources::manager().getProgramCompute("irradiance_compute");

	_shCoeffs.reset(new Buffer(9 * sizeof(glm::vec4), BufferType::STORAGE));
	std::vector<glm::vec4> coeffs(9, glm::vec4(0.0f));
	_shCoeffs->upload(coeffs);

	// Compute the camera for each face.
	for(uint i = 0; i < 6; ++i) {
		_cameras[i].pose(_position, _position + Library::boxCenters[i], Library::boxUps[i]);
		_cameras[i].projection(1.0f, glm::half_pi<float>(), clippingPlanes[0], clippingPlanes[1]);
	}

	probe.registerEnvironment(_framebuffer->texture(), _shCoeffs);
}

void Probe::update(uint budget){
	// Simple state machine:
	// (draw a face) ^ 6 -> ((convolve a face) ^ 6) ^ (mip count)) -> (dispatch irradiance compute)

	// Follow steps while we have budget.
	while(budget > 0){
		switch (_currentState) {
			case ProbeState::DRAW_FACES:
				// Draw the current face.
				_renderer->draw(_cameras[_substepDraw], *_framebuffer, _substepDraw);
				++_substepDraw;
				// If all faces done, reset and move to radiance estimation.
				if(_substepDraw >= 6){
					_substepDraw = 0;
					_currentState = ProbeState::CONVOLVE_RADIANCE;
				}
				break;

			case ProbeState::CONVOLVE_RADIANCE:
				// Generate a level of the radiance.
				convolveRadiance(1.2f, _substepRadiance);
				++_substepRadiance;
				// If all levels done, reset and move to irradiance integration.
				if(_substepRadiance >= _framebuffer->texture()->levels){
					// No need to filter level 0.
					_substepRadiance = 1;
					_currentState = ProbeState::GENERATE_IRRADIANCE;
				}
				break;

			case ProbeState::GENERATE_IRRADIANCE:
				// Generate irradiance.
				estimateIrradiance(5.0f);
				_currentState = ProbeState::DRAW_FACES;
				break;

			default:
				break;
		}
		--budget;
	}
}

void Probe::convolveRadiance(float clamp, uint level) {

	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	_radianceCompute->use();
	_radianceCompute->uniform("clampMax", clamp);

	const uint wh		   = _framebuffer->texture()->width / (1 << level);
	const float roughness  = float(level) / float(_framebuffer->texture()->levels - 1u);
	const int samplesCount = 64;

	_radianceCompute->uniform("mipmapRoughness", roughness);
	_radianceCompute->uniform("samplesCount", samplesCount);
	_radianceCompute->texture(_framebuffer->texture(), 0, uint(level)-1u);
	_radianceCompute->texture(_framebuffer->texture(), 1, uint(level));
	GPU::dispatch(wh, wh, 6);

}

void Probe::estimateIrradiance(float clamp) {
	// Downscale radiance to a smaller texture.
	for(uint lid = 0; lid < 6; ++lid) {
		GPU::blit(*_framebuffer, *_copy, lid, lid, 0, 0, Filter::LINEAR);
	}
	// Dispatch pr-face coefficients accumulation and reduction/SH projection.
	_irradianceCompute->use();
	_irradianceCompute->texture(*_copy->texture(), 0);
	_irradianceCompute->buffer(*_shCoeffs, 0);
	_irradianceCompute->uniform("clamp", clamp);
	_irradianceCompute->uniform("side", _copy->width());
	GPU::dispatch(1, 1, 1);

}

void Probe::extractIrradianceSHCoeffs(const Texture & cubemap, float clamp, std::vector<glm::vec3> & shCoeffs) {
	shCoeffs.resize(9);

	// Indices conversions from cubemap UVs to direction.
	static const std::vector<int> axisIndices  = {0, 0, 1, 1, 2, 2};
	static const std::vector<float> axisMul	   = {1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f};
	static const std::vector<int> horizIndices = {2, 2, 0, 0, 0, 0};
	static const std::vector<float> horizMul   = {-1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
	static const std::vector<int> vertIndices  = {1, 1, 2, 2, 1, 1};
	static const std::vector<float> vertMul	   = {1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};

	// Spherical harmonics coefficients.
	std::array<glm::vec3, 9> LCoeffs = {};
	LCoeffs.fill(glm::vec3(0.0f, 0.0f, 0.0f));
	const float y0 = 0.282095f;
	const float y1 = 0.488603f;
	const float y2 = 1.092548f;
	const float y3 = 0.315392f;
	const float y4 = 0.546274f;

	float denom		= 0.0f;
	const uint side = cubemap.width;
	for(uint i = 0; i < 6; ++i) {
		const auto & currentSide = cubemap.images[i];
		for(uint y = 0; y < side; ++y) {
			for(uint x = 0; x < side; ++x) {

				const float v		 = -1.0f + 1.0f / float(side) + float(y) * 2.0f / float(side);
				const float u		 = -1.0f + 1.0f / float(side) + float(x) * 2.0f / float(side);
				glm::vec3 pos		 = glm::vec3(0.0f, 0.0f, 0.0f);
				pos[axisIndices[i]]	 = axisMul[i];
				pos[horizIndices[i]] = horizMul[i] * u;
				pos[vertIndices[i]]	 = vertMul[i] * v;
				pos					 = glm::normalize(pos);
				// Normalization factor.
				const float fTmp   = 1.0f + u * u + v * v;
				const float weight = 4.0f / (sqrt(fTmp) * fTmp);
				denom += weight;
				// HDR color.
				const glm::vec3 & rgb = currentSide.rgb(x, y);
				const glm::vec3 hdr = weight * glm::min(rgb, clamp);
				// Y0,0  = 0.282095
				LCoeffs[0] += hdr * y0;
				// Y1,-1 = 0.488603 y
				LCoeffs[1] += hdr * (y1 * pos[1]);
				// Y1,0  = 0.488603 z
				LCoeffs[2] += hdr * (y1 * pos[2]);
				// Y1,1  = 0.488603 x
				LCoeffs[3] += hdr * (y1 * pos[0]);
				// Y2,-2 = 1.092548 xy
				LCoeffs[4] += hdr * (y2 * (pos[0] * pos[1]));
				// Y2,-1 = 1.092548 yz
				LCoeffs[5] += hdr * (y2 * pos[1] * pos[2]);
				// Y2,0  = 0.315392 (3z^2 - 1)
				LCoeffs[6] += hdr * (y3 * (3.0f * pos[2] * pos[2] - 1.0f));
				// Y2,1  = 1.092548 xz
				LCoeffs[7] += hdr * (y2 * pos[0] * pos[2]);
				// Y2,2  = 0.546274 (x^2 - y^2)
				LCoeffs[8] += hdr * (y4 * (pos[0] * pos[0] - pos[1] * pos[1]));
			}
		}
	}
	// Normalization.
	for(auto & coeff : LCoeffs) {
		coeff *= 4.0 / denom;
	}
	// To go from radiance to irradiance, we need to apply a cosine lobe convolution on the sphere in spatial domain.
	// This can be expressed as a product in frequency (on the SH basis) domain, with constant pre-computed coefficients.
	// See:	Ramamoorthi, Ravi, and Pat Hanrahan. "An efficient representation for irradiance environment maps."
	//		Proceedings of the 28th annual conference on Computer graphics and interactive techniques. ACM, 2001.
	const float c1 = 0.429043f;
	const float c2 = 0.511664f;
	const float c3 = 0.743125f;
	const float c4 = 0.886227f;
	const float c5 = 0.247708f;
	shCoeffs[0]	   = c4 * LCoeffs[0] - c5 * LCoeffs[6];
	shCoeffs[1]	   = 2.0f * c2 * LCoeffs[1];
	shCoeffs[2]	   = 2.0f * c2 * LCoeffs[2];
	shCoeffs[3]	   = 2.0f * c2 * LCoeffs[3];
	shCoeffs[4]	   = 2.0f * c1 * LCoeffs[4];
	shCoeffs[5]	   = 2.0f * c1 * LCoeffs[5];
	shCoeffs[6]	   = c3 * LCoeffs[6];
	shCoeffs[7]	   = 2.0f * c1 * LCoeffs[7];
	shCoeffs[8]	   = c1 * LCoeffs[8];
}

uint Probe::totalBudget() const {
	/* draw faces + convolve each level + generate irradiance */
	return 6 + _framebuffer->texture()->levels + 1;
}
