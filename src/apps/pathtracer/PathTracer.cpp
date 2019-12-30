#include "PathTracer.hpp"
#include "system/System.hpp"
#include "system/Random.hpp"
#include <chrono>

PathTracer::PathTracer(const std::shared_ptr<Scene> & scene) {
	// Add all scene objects to the raycaster.
	for(const auto & obj : scene->objects) {
		_raycaster.addMesh(*obj.mesh(), obj.model());
	}
	_raycaster.updateHierarchy();
	_scene = scene;
}

void PathTracer::render(const Camera & camera, size_t samples, size_t depth, Image & render) {

	// Safety checks.
	if(!_scene) {
		Log::Error() << "[PathTracer] No scene available." << std::endl;
		return;
	}
	if(render.components != 3) {
		Log::Warning() << "[PathTracer] Expected a RGB image." << std::endl;
	}
	const size_t samplesOld = samples;
	samples					= size_t(std::pow(2, std::round(std::log2(float(samplesOld)))));
	if(samplesOld != samples) {
		Log::Warning() << "[PathTracer] Non power-of-2 samples count. Using " << samples << " instead." << std::endl;
	}

	// Compute incremental pixel shifts.
	glm::vec3 corner, dx, dy;
	camera.pixelShifts(corner, dx, dy);

	// Prepare the stratified grid.
	// We know that we have 2^k samples.
	const int k = int(std::floor(std::log2(samples)));
	// If even, just use k samples on each side.
	glm::ivec2 stratesCount(int(std::pow(2, k / 2)));
	if(k % 2 == 1) {
		//  Else dispatch the extraneous factor of 2 on the horizontal axis.
		stratesCount[0] = int(std::pow(2, (k + 1) / 2));
		stratesCount[1] = int(std::pow(2, (k - 1) / 2));
	}
	const glm::vec2 stratesSize = 1.0f / glm::vec2(stratesCount);

	// Start chrono.
	const auto start = std::chrono::steady_clock::now();

	// Parallelize on each row of the image.
	System::forParallel(0, size_t(render.height), [&render, samples, stratesCount, &stratesSize, &corner, &dx, &dy, &camera, depth, this](size_t y) {
		for(size_t x = 0; x < size_t(render.width); ++x) {
			for(size_t sid = 0; sid < samples; ++sid) {
				// Find the grid location.
				const int sidy = int(sid) / stratesCount.x;
				const int sidx = int(sid) % stratesCount.x;

				// Draw random shift in [0.0,1.0f) for jittering.
				const float jx = Random::Float();
				const float jy = Random::Float();
				// Compute position in the stratification grid.
				const glm::vec2 gridPos = glm::vec2(float(sidx) + jx, float(sidy) + jy);
				// Position in screen space.
				const glm::vec2 screenPos = gridPos * stratesSize + glm::vec2(x, y);

				// Derive a position on the image plane from the pixel.
				const glm::vec2 ndcPos = screenPos / glm::vec2(render.width, render.height);
				// Place the point on the near plane in clip space.
				const glm::vec3 worldPos = corner + ndcPos.x * dx + ndcPos.y * dy;

				glm::vec3 rayPos = camera.position();
				glm::vec3 rayDir = glm::normalize(worldPos - camera.position());

				glm::vec3 sampleColor(0.0f);
				glm::vec3 attenuation(1.0f);

				for(size_t did = 0; did < depth; ++did) {
					// Query closest intersection.
					const Raycaster::RayHit hit = _raycaster.intersects(rayPos, rayDir);
					
					 
					// If no hit, background.
					if(!hit.hit) {
						const Scene::Background mode = _scene->backgroundMode;

						// If direct background hit, produce the correct color without attenuation.
						if(did == 0) {
							if(mode == Scene::Background::IMAGE) {
								const Image & image = _scene->background->textures()[0]->images[0];
								sampleColor			= image.rgbl(ndcPos.x, ndcPos.y);
							} else if(mode == Scene::Background::SKYBOX) {
								const Texture * tex = _scene->background->textures()[0];
								sampleColor			= tex->sampleCubemap(glm::normalize(rayDir));
							} else {
								sampleColor = _scene->backgroundColor;
							}
							// \todo Support sampling atmospheric simulation.
							break;
						}

						// Else, only environment maps contribute to indirect illumination (for now).
						if(mode == Scene::Background::SKYBOX) {
							const Texture * tex = _scene->background->textures()[0];
							sampleColor += attenuation * tex->sampleCubemap(glm::normalize(rayDir));
						}
						break;
					}

					glm::vec3 illumination(0.0f);

					// Fetch geometry infos...
					const Object & obj = _scene->objects[hit.meshId];
					const Mesh & mesh  = *obj.mesh();
					const glm::vec3 p  = rayPos + hit.dist * rayDir;
					const glm::vec3 n  = glm::normalize(glm::inverse(glm::transpose(obj.model())) * glm::vec4(Raycaster::interpolateNormal(hit, mesh), 0.0));
					const glm::vec2 uv = Raycaster::interpolateUV(hit, mesh);
					
					// No support for geometric emitters for now.
					
					// Compute lighting.
					// Cast a ray toward one of the lights, at random.
					if(!_scene->lights.empty()){
						const unsigned int lid = Random::Int(0, _scene->lights.size()-1);
						const auto & light = _scene->lights[lid];
						glm::vec3 direction;
						float falloff;
						if(light->visible(p+0.001f*n, _raycaster, direction, falloff)) {
							const float diffuse = glm::max(glm::dot(n, direction), 0.0f);
							illumination += falloff * diffuse * light->intensity();
						}
					}
					// Because we only have analytical lights, we can't hit an emitter via the raycaster, so no double-hit case to consider for now.

					// Fetch base color from texture.
					const Image & image  = obj.textures()[0]->images[0];
					const glm::vec4 bCol = image.rgbal(uv.x, uv.y);
					// In case of alpha cut-out, just update the position to the intersection and keep casting.
					// The 'mini' margin will ensures that we don't reintersect the same surface.
					if(bCol.a < 0.01f) {
						rayPos = p;
						continue;
					}
					const glm::vec3 baseColor = glm::pow(glm::vec3(bCol), glm::vec3(2.2f));

					// Bounce decay.
					attenuation *= baseColor;
					sampleColor += attenuation * illumination;

					// Update position and ray direction.
					if(did < depth - 1) {
						rayPos = p;
						// For the direction, we want to sample the hemisphere, weighted by the cosine weight to better use our samples.
						// We use the trick described by Peter Shirley in 'Raytracing in One Week-End':
						// Uniformly sample a sphere tangent to the surface, add this to the normal.
						rayDir = glm::normalize(n + Random::sampleSphere());
					}
				}
				// Modulate and store.
				render.rgb(int(x), int(y)) += glm::min(sampleColor, 4.0f);
			}
		}
	});

	// Normalize and gamma correction.
	System::forParallel(0, size_t(render.height), [&render, &samples](size_t y) {
		for(size_t x = 0; x < (render.width); ++x) {
			const glm::vec3 color	  = render.rgb(int(x), int(y)) / float(samples);
			render.rgb(int(x), int(y)) = glm::pow(color, glm::vec3(1.0f / 2.2f));
		}
	});

	// Display duration.
	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	Log::Info() << "[PathTracer] Rendering took " << duration.count() / 1000.0f << "s at " << render.width << "x" << render.height << "." << std::endl;
}
