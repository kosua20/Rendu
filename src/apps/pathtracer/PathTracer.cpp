#include "PathTracer.hpp"
#include "MaterialGGX.hpp"
#include "MaterialSky.hpp"

#include "scene/Sky.hpp"

#include "system/System.hpp"
#include "generation/Random.hpp"
#include "system/Query.hpp"

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

glm::vec3 PathTracer::evalBackground(const glm::vec3 & rayDir, const glm::vec3 & rayPos, const glm::vec2 & ndcPos, bool directHit) const {
	const Scene::Background mode = _scene->backgroundMode;

	glm::vec3 color(0.0f);
	const Material& material = _scene->background->material();

	// If direct background hit, produce the correct color without attenuation.
	if(directHit) {
		if(mode == Scene::Background::IMAGE) {
			const Image & image = material.textures()[0]->images[0];
			color = image.rgbl(ndcPos.x, ndcPos.y);
		} else if(mode == Scene::Background::SKYBOX) {
			const Texture * tex = material.textures()[0];
			color = tex->sampleCubemap(glm::normalize(rayDir));
		} else if(mode == Scene::Background::ATMOSPHERE) {
			const glm::vec3 & sunDir = dynamic_cast<const Sky *>(_scene->background.get())->direction();
			color = MaterialSky::eval(rayPos, glm::normalize(rayDir), sunDir);
		} else {
			color = _scene->backgroundColor;
		}
		return color;
	}

	// Else, only environment maps and atmospheric simulations contribute to indirect illumination.
	if(mode == Scene::Background::SKYBOX) {
		const Texture * tex = material.textures()[0];
		color = tex->sampleCubemap(glm::normalize(rayDir));
	} else if(mode == Scene::Background::ATMOSPHERE) {
		const glm::vec3 & sunDir = dynamic_cast<const Sky *>(_scene->background.get())->direction();
		color = MaterialSky::eval(rayPos, glm::normalize(rayDir), sunDir);
	}
	return color;
}

glm::ivec2 PathTracer::getSampleGrid(size_t samples) {
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
	return stratesCount;
}

glm::vec2 PathTracer::getSamplePosition(size_t sid, const glm::ivec2 & cellCount, const glm::vec2 & cellSize) {
	// Find the grid location.
	const int sidy = int(sid) / cellCount.x;
	const int sidx = int(sid) % cellCount.x;

	// Draw random shift in [0.0,1.0f) for jittering.
	const float jx = Random::Float();
	const float jy = Random::Float();
	// Compute position in the stratification grid.
	const glm::vec2 gridPos = glm::vec2(float(sidx) + jx, float(sidy) + jy);
	// Position in screen space.
	const glm::vec2 localPos = gridPos * cellSize;
	return localPos;
}

glm::mat3 PathTracer::buildLocalFrame(const Object & obj, const Raycaster::Hit & hit, const glm::vec3 & rayDir, const glm::vec2 & uv){
	const auto & mesh = *obj.mesh();
	const glm::vec3 n = glm::normalize(Raycaster::interpolateAttribute(hit, mesh, mesh.normals));
	glm::vec3 t = glm::normalize(Raycaster::interpolateAttribute(hit, mesh, mesh.tangents));
	// Ensure that the resulting frame is orthogonal.
	const glm::vec3 b = glm::normalize(glm::cross(n, t));
	t = glm::normalize(glm::cross(b, n));
	// Convert to world frame.
	const glm::mat3 invtp = glm::inverse(glm::transpose(glm::mat3(obj.model())));
	// From tangent space to world space.
	glm::mat3 tbn;
	tbn[0] = glm::normalize(glm::vec3(invtp * glm::vec4(t, 0.0f)));
	tbn[1] = glm::normalize(glm::vec3(invtp * glm::vec4(b, 0.0f)));
	tbn[2] = glm::normalize(glm::vec3(invtp * glm::vec4(n, 0.0f)));

	// Flip normal if needed (all objects are double sided).
	const bool frontFacing = glm::dot(tbn[2], rayDir) < 0.0f;
	if(!frontFacing){
		tbn[2] *= -1.0f;
	}

	// If we have a normal map, perturb the local normal and udpate the frame.
	if(obj.useTexCoords() && obj.material().type() != Material::Type::Emissive){
		const Material& mat = obj.material();
		const glm::vec3 imgNormal = glm::vec3(mat.textures()[1]->images[0].rgbal(uv.x, uv.y));
		const glm::vec3 localNormal = glm::normalize(2.0f * imgNormal - 1.0f);
		// Convert local normal to world.
		const glm::vec3 nn = glm::normalize(tbn * localNormal);
		const glm::vec3 bn = glm::normalize(glm::cross(nn, tbn[0]));
		const glm::vec3 tn = glm::normalize(glm::cross(bn, nn));
		tbn = glm::mat3(tn, bn, nn);
	}
	return tbn;
}

bool PathTracer::checkVisibility(const glm::vec3 & startPos, const glm::vec3 & rayDir, float maxDist) const {
	glm::vec3 lpos = startPos;
	// Walk along the ray, checking at each intersection if there is occlusion.
	while(maxDist > 0.0f){
		const Raycaster::Hit lhit = _raycaster.intersects(lpos, rayDir, 0.001f, maxDist);
		// No hit, we are visible, exit.
		if(!lhit.hit){
			return true;
		}
		// If we hit, two cases.
		const auto & lobj = _scene->objects[lhit.meshId];
		const Material& lmat = lobj.material();
		if(!lmat.masked() || !lobj.useTexCoords()){
			// If the object has no mask or no uvs, geometric occlusion is always valid.
			return false;
		} else {
			// We have to sample the object alpha mask.
			// For this we compute the UVs and check the texture.
			const auto & lmesh = *lobj.mesh();
			const glm::vec2 luv = Raycaster::interpolateAttribute(lhit, lmesh, lmesh.texcoords);
			const float alpha = lmat.textures()[0]->images[0].rgbal(luv.x, luv.y).a;
			if(alpha < 0.01f){
				// Transparent: shift, update the distance and keep casting.
				maxDist = maxDist - lhit.dist;
				lpos = lpos + lhit.dist * rayDir;
			} else {
				// Occlusion.
				return false;
				break;
			}
		}
	}
	return true;
}

void PathTracer::render(const Camera & camera, size_t samples, size_t depth, Image & render) {

	// Safety checks.
	if(!_scene) {
		Log::Error() << "[PathTracer] No scene available." << std::endl;
		return;
	}
	if(render.components < 3) {
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
	const glm::ivec2 cellCount = getSampleGrid(samples);
	const glm::vec2 cellSize = 1.0f / glm::vec2(cellCount);

	// Start chrono.
	Query timer;
	timer.begin();

	// Parallelize on each row of the image.
	System::forParallel(0, size_t(render.height), [&render, samples, &cellCount, &cellSize, &corner, &dx, &dy, &camera, depth, this](size_t y) {
		for(size_t x = 0; x < size_t(render.width); ++x) {
			for(size_t sid = 0; sid < samples; ++sid) {

				// Get the position of the sample in screenspace.
				const glm::vec2 screenPos = glm::vec2(x, y) + getSamplePosition(sid, cellCount, cellSize);
				// Derive a position on the image plane from the pixel.
				const glm::vec2 ndcPos = screenPos / glm::vec2(render.width, render.height);
				// Place the point on the near plane in clip space.
				const glm::vec3 worldPos = corner + ndcPos.x * dx + ndcPos.y * dy;
				// Initial ray setup.
				glm::vec3 rayPos = camera.position();
				glm::vec3 rayDir = glm::normalize(worldPos - camera.position());
				glm::vec3 sampleColor(0.0f);
				glm::vec3 attenuation(1.0f);

				for(size_t did = 0; did < depth; ++did) {
					// Query closest intersection.
					const Raycaster::Hit hit = _raycaster.intersects(rayPos, rayDir);
					// If no hit, background.
					if(!hit.hit) {
						sampleColor += attenuation * evalBackground(rayDir, rayPos, ndcPos, did == 0);
						break;
					}

					// Fetch geometry infos...
					const Object & obj = _scene->objects[hit.meshId];
					const Mesh & mesh  = *obj.mesh();
					const glm::vec3 p  = rayPos + hit.dist * rayDir;
					// Fetch material texel information.
					const bool noUVs = !obj.useTexCoords();
					const glm::vec2 uv = noUVs ? glm::vec2(0.5f, 0.5f) :  Raycaster::interpolateAttribute(hit, mesh, mesh.texcoords);
					const Material& mat = obj.material();
					const Image & image  = mat.textures()[0]->images[0];
					const glm::vec4 bCol = image.rgbal(uv.x, uv.y);
					// In case of alpha cut-out, just update the position to the intersection and keep casting.
					// The 'mini' margin will ensures that we don't reintersect the same surface.
					if(mat.masked() && bCol.a < 0.01f) {
						rayPos = p;
						continue;
					}
					// For emissive we don't apply any BRDF or re-cast rays, we just receive emitted light.
					if(mat.type() == Material::Type::Emissive){
						// Should we gamma-correct emissive textures?
						sampleColor += attenuation * glm::vec3(bCol);
						// No need to continue further.
						break;
					}

					// Compute local tangent frame.
					const glm::mat3 tbn = buildLocalFrame(obj, hit, rayDir, uv);
					const glm::mat3 itbn = glm::transpose(tbn);
					// For sampling and evaluating the BRDF, convert outgoing direction to the local frame.
					const glm::vec3 wo = glm::normalize(itbn * (-rayDir));
					const glm::vec3 baseColor = glm::pow(glm::vec3(bCol), glm::vec3(2.2f));
					// Check other material attributes.
					const Image & imageRMAO  = mat.textures()[2]->images[0];
					const glm::vec4 rmao = imageRMAO.rgbal(uv.x, uv.y);

					// Direct light sampling.
					if(!_scene->lights.empty()){
						// Take a light at random.
						const unsigned int lid = Random::Int(0, int(_scene->lights.size()-1));
						const auto & light = _scene->lights[lid];
						// Shift slightly to avoid grazing angle self-intersections.
						const glm::vec3 pShift = p+0.001f*tbn[2];
						// Sample a ray going from the surface of the object to the light.
						float maxDist, falloff;
						const glm::vec3 direction = light->sample(pShift, maxDist, falloff);
						// Test visibility of needed..
						bool visible = falloff > 0.0f;
						if(visible && light->castsShadow()){
							visible = checkVisibility(pShift, direction, maxDist);
						}

						// If visible, add contribution weighted by the surface BRDF.
						if(visible){
							const glm::vec3 lwi = glm::normalize(itbn * direction);
							const glm::vec3 evalLight = MaterialGGX::eval(wo, baseColor, rmao.r, rmao.g, lwi);
							const float lightPdf = 1.0f / float(_scene->lights.size());
							const glm::vec3 illumination = falloff * evalLight * light->intensity() / lightPdf;
							// Because we only sample analytical lights, we can't hit an emitter via the raycaster, so no double-hit case to consider for now.
							sampleColor += attenuation * illumination;
						}
					}

					// Pick next direction based on the BRDF.
					glm::vec3 wi;
					glm::vec3 eval = MaterialGGX::sampleAndEval(wo, baseColor, rmao.r, rmao.g, wi);
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
			// Normalize.
			const glm::vec3 color	  = render.rgb(int(x), int(y)) / float(samples);
			render.rgb(int(x), int(y)) = color;
		}
	});

	// Display duration.
	timer.end();
	Log::Info() << "[PathTracer] Rendering took " << float(timer.value()) / 1000000000.0f << "s at " << render.width << "x" << render.height << "." << std::endl;
}
