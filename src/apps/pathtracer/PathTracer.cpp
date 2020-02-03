#include "PathTracer.hpp"
#include "system/System.hpp"
#include "system/Random.hpp"
#include <chrono>

PathTracer::PathTracer(const std::shared_ptr<Scene> & scene) {
	// Add all scene objects to the raycaster.
	for(const auto & obj : scene->objects) {
		if(obj.mesh()->tangents.empty()){
			Log::Error() << "The path tracer requires local tangent frames for all meshes." << std::endl;
		}
		_raycaster.addMesh(*obj.mesh(), obj.model());
	}
	_raycaster.updateHierarchy();
	_scene = scene;
}


glm::vec3 F(glm::vec3 F0, float VdotH){
	return F0 + std::pow(1.0f - VdotH, 5.0f) * (glm::vec3(1.0f) - F0);
}

float D(float NdotH, float alpha){
	const float halfDenum = NdotH * NdotH * (alpha * alpha - 1.0f) + 1.0f;
	const float halfTerm = alpha / std::max(0.0001f, halfDenum);
	return halfTerm * halfTerm * glm::one_over_pi<float>();
}

float V(float NdotL, float NdotV, float alpha){
	// Correct version.
	const float alpha2 = alpha * alpha;
	const float visL = NdotV * std::sqrt((-NdotL * alpha2 + NdotL) * NdotL + alpha2);
	const float visV = NdotL * std::sqrt((-NdotV * alpha2 + NdotV) * NdotV + alpha2);
	return 0.5f / std::max(0.0001f, visV + visL);
}


glm::vec3 sampleBrdf(const glm::vec3 & wo, const glm::vec3 & baseColor, float roughness, float metallic, glm::vec3 & wi){
	const float probaSpecular = glm::mix(1.0f / (glm::dot(baseColor, glm::vec3(1.0f)) / 3.0f + 1.0f), 1.0f,  metallic);

	const float roughClamp = std::max(0.045f, roughness);
	const float alpha = std::max(0.0001f, roughClamp*roughClamp);

	if(Random::Float() < probaSpecular){
		// Sample specular lobe.
		const float a2 = alpha * alpha;
		const float x = Random::Float();
		// for dielectrics, Walter et al. have a roughness rescaling hack.
		// alpha * (1.2f - 0.2f * std::sqrt(std::abs(wi.z)));
		const float phiH = Random::Float() * glm::two_pi<float>();
		const float cosThetaHSqr = std::min((1.0f - x) / ((a2 - 1.0f) * x + 1.0f), 1.0f);
		const float cosThetaH = std::sqrt(cosThetaHSqr);
		const float sinThetaH = std::sqrt(1.0f - cosThetaHSqr);
		const glm::vec3 lh(sinThetaH * cos(phiH), sinThetaH * sin(phiH), cosThetaH);
		// wi is outgoing here. wo is outgoing also.
		wi = 2.0f * glm::dot(wo, lh) * lh - wo;
	} else {
		// Else sample diffuse lobe.
		wi = Random::sampleCosineHemisphere();
		if(wo.z < 0.0f){
			wi.z *= -1.0f;
		}
	}
	if(wi.z < 0.0f){
		return glm::vec3(0.0f);
	}
	const glm::vec3 h = glm::normalize(wi + wo);
	const float NdotH = std::max(h.z, 0.0f);
	const float VdotH = std::max(glm::dot(wi,h), 0.0f);
	const float NdotL = std::max(wo.z, 0.0f);
	const float NdotV = std::max(wi.z, 0.0f);

	// Evaluate D(h)
	const float Dh = D(NdotH, alpha);
	// Evaluate the total PDF.
	const float hPdf = Dh * NdotH;
	const float pdf = glm::mix(glm::one_over_pi<float>() * NdotV, hPdf / (4.0f * std::max(0.0001f, VdotH)), probaSpecular);
	if(pdf == 0.0f){
		return glm::vec3(0.0f);
	}
	// Evaluate the total BRDF and weight it.
	const glm::vec3 F0 = glm::mix(glm::vec3(0.04f), baseColor, metallic);
	const glm::vec3 specular = Dh * V(NdotL, NdotV, alpha) * F(F0, VdotH);

	const glm::vec3 diffuse = (1.0f - metallic) * glm::one_over_pi<float>() * baseColor * (1.0f - F0);
	// Multi scattering adjustment hack.
	const glm::vec3 multiAdj = glm::vec3(1.0f) + (2.0f * alpha * alpha * NdotV) * F0;
	const glm::vec3 brdf = (diffuse + specular * multiAdj) * NdotV;
	return brdf / pdf;
}

glm::vec3 brdf(const glm::vec3 & wo, const glm::vec3 & baseColor, float roughness, float metallic, const glm::vec3 & wi){

	const float roughClamp = std::max(0.045f, roughness);
	const float alpha = std::max(0.0001f, roughClamp*roughClamp);

	if(wi.z < 0.0f){
		return glm::vec3(0.0f);
	}
	const glm::vec3 h = glm::normalize(wi + wo);
	const float NdotH = std::max(h.z, 0.0f);
	const float VdotH = std::max(glm::dot(wi,h), 0.0f);
	const float NdotL = std::max(wo.z, 0.0f);
	const float NdotV = std::max(wi.z, 0.0f);

	// Evaluate D(h)
	const float Dh = D(NdotH, alpha);

	// Evaluate the total BRDF and weight it.
	const glm::vec3 F0 = glm::mix(glm::vec3(0.04f), baseColor, metallic);
	const glm::vec3 specular = Dh * V(NdotL, NdotV, alpha) * F(F0, VdotH);

	const glm::vec3 diffuse = (1.0f - metallic) * glm::one_over_pi<float>() * baseColor * (1.0f - F0);
	// Multi scattering adjustment hack.
	const glm::vec3 multiAdj = glm::vec3(1.0f) + (2.0f * alpha * alpha * NdotV) * F0;
	const glm::vec3 brdf = (diffuse + specular * multiAdj) * NdotV;
	return brdf;
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

					// Fetch geometry infos...
					const Object & obj = _scene->objects[hit.meshId];
					const Mesh & mesh  = *obj.mesh();
					const glm::vec3 p  = rayPos + hit.dist * rayDir;
					const glm::mat4 invtp = glm::inverse(glm::transpose(obj.model()));
					glm::vec3 n = glm::normalize(Raycaster::interpolateAttribute(hit, mesh, mesh.normals));
					glm::vec3 t = glm::normalize(Raycaster::interpolateAttribute(hit, mesh, mesh.tangents));
					// Convert to world frame.
					// \todo Support the different types of materials (double sided for instance).
					n = glm::normalize(glm::vec3(invtp * glm::vec4(n, 0.0f)));
					t = glm::normalize(glm::vec3(invtp * glm::vec4(t, 0.0f)));
					// Enforce orthogonality.
					const glm::vec3 b = glm::normalize(glm::cross(n,t));
					t = glm::normalize(glm::cross(b,n));

					const glm::mat3 tbn(t, b, n);
					const glm::mat3 itbn = glm::transpose(tbn);
					const glm::vec2 uv = Raycaster::interpolateAttribute(hit, mesh, mesh.texcoords);


					// Fetch base color from texture.
					const Image & image  = obj.textures()[0]->images[0];
					const glm::vec4 bCol = image.rgbal(uv.x, uv.y);
					const Image & imageRMAO  = obj.textures()[2]->images[0];
					const glm::vec4 rmao = imageRMAO.rgbal(uv.x, uv.y);
					// In case of alpha cut-out, just update the position to the intersection and keep casting.
					// The 'mini' margin will ensures that we don't reintersect the same surface.
					if(bCol.a < 0.01f) {
						rayPos = p;
						continue;
					}

					// When sampling and evaluating the BRDF, work in the local (t,b,n) frame.

					const glm::vec3 wo = glm::normalize(itbn * (-rayDir));
					const glm::vec3 baseColor = glm::pow(glm::vec3(bCol), glm::vec3(2.2f));

					// Direct light sampling.
					// Cast a ray toward one of the lights, at random.
					if(!_scene->lights.empty()){
						// No support for geometric emitters for now.
						const unsigned int lid = Random::Int(0, _scene->lights.size()-1);
						const auto & light = _scene->lights[lid];
						glm::vec3 direction;
						float falloff;
						if(light->visible(p+0.001f*n, _raycaster, direction, falloff)) {
							const glm::vec3 lwi = glm::normalize(itbn * (direction));
							const glm::vec3 evalLight = brdf(wo, baseColor, rmao.r, rmao.g, lwi);
							const float lightPdf = 1.0f / float(_scene->lights.size());
							const glm::vec3 illumination = falloff * evalLight * light->intensity() / lightPdf;
							// Because we only sample analytical lights, we can't hit an emitter via the raycaster, so no double-hit case to consider for now.
							sampleColor += attenuation * illumination;
						}
					}

					// Pick next direction based on the BRDF.
					glm::vec3 wi;
					glm::vec3 eval = sampleBrdf(wo, baseColor, rmao.r, rmao.g, wi);
					const glm::vec3 nextRayDir = glm::normalize(tbn * wi);
					// Bounce decay.
					attenuation *= eval;

					// Update position and ray direction.
					if(did < depth - 1) {
						rayPos = p;
						rayDir = glm::normalize(nextRayDir);
					}
				}
				// Clamp and store.
				render.rgb(int(x), int(y)) += glm::min(sampleColor, 5.0f);
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
