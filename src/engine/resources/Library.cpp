#include "resources/Library.hpp"
#include "resources/Bounds.hpp"

const std::array<glm::vec3, 6> Library::boxUps = {
	glm::vec3(0.0f, -1.0f, 0.0f),
	glm::vec3(0.0f, -1.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, -1.0f),
	glm::vec3(0.0f, 0.0f, 1.0f),
	glm::vec3(0.0f, -1.0f, 0.0f),
	glm::vec3(0.0f, -1.0f, 0.0f)};

const std::array<glm::vec3, 6> Library::boxCenters = {
	glm::vec3(1.0, 0.0, 0.0),
	glm::vec3(-1.0, 0.0, 0.0),
	glm::vec3(0.0, -1.0, 0.0),
	glm::vec3(0.0, 1.0, 0.0),
	glm::vec3(0.0, 0.0, 1.0),
	glm::vec3(0.0, 0.0, -1.0)
};

const std::array<glm::vec3, 6> Library::boxRights = {
	glm::vec3( 0.0f, 0.0f, -1.0f),
	glm::vec3( 0.0f, 0.0f,  1.0f),
	glm::vec3( 1.0f, 0.0f,  0.0f),
	glm::vec3( 1.0f, 0.0f,  0.0f),
	glm::vec3( 1.0f, 0.0f,  0.0f),
	glm::vec3(-1.0f, 0.0f,  0.0f)
};

const std::array<glm::mat4, 6> Library::boxVs = {
	glm::lookAt(glm::vec3(0.0f), boxCenters[0], boxUps[0]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[1], boxUps[1]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[2], boxUps[2]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[3], boxUps[3]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[4], boxUps[4]),
	glm::lookAt(glm::vec3(0.0f), boxCenters[5], boxUps[5])
};

const std::array<glm::mat4, 6> Library::boxVPs = {
	Frustum::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[0],
	Frustum::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[1],
	Frustum::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[2],
	Frustum::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[3],
	Frustum::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[4],
	Frustum::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * boxVs[5]
};


Mesh Library::generateGrid(uint resolution, float scale){

	const int res = int(resolution);
	const int hres = (res-1)/2;

	Mesh mesh("Grid-" + std::to_string(resolution) + "-" + std::to_string(scale));
	mesh.positions.reserve(res * res);
	mesh.texcoords.reserve(res * res);
	mesh.normals.reserve(res * res);
	mesh.indices.reserve((res-1)*(res-1)*2*3);

	for(int z = 0; z < res; ++z){
		for(int x = 0; x < res; ++x){
			mesh.positions.emplace_back(scale * float(x-hres), 0.0f, scale * float(z-hres));
			mesh.texcoords.emplace_back(float(x)/float(res-1), float(z)/float(res-1));
			mesh.normals.emplace_back(0.0f, 1.0f, 0.0);
			if(x < (res-1) && z < (res-1)){
				const uint xid = z * res + x;
				mesh.indices.push_back(xid);
				mesh.indices.push_back(xid + res);
				mesh.indices.push_back(xid + res + 1);
				mesh.indices.push_back(xid + 1);
			}
		}
	}
	return mesh;
}

Mesh Library::generateCylinder(uint resolution, float radius, float height){
	Mesh mesh("Cylinder-" + std::to_string(resolution) + "-" + std::to_string(radius) + "-" + std::to_string(height));
	mesh.positions.reserve(2 * resolution);
	mesh.texcoords.reserve(2 * resolution);
	mesh.normals.reserve(2 * resolution);
	mesh.indices.reserve(2 * resolution * 3);

	const float y = 0.5f * height;
	for(uint i = 0; i < resolution; ++i){
		const float angle = float(i) / float(resolution) * glm::two_pi<float>();
		const float x = radius * std::cos(angle);
		const float z = radius * std::sin(angle);
		mesh.positions.emplace_back(x, -y, z);
		mesh.positions.emplace_back(x,  y, z);
		const float u = float(i)/float(resolution);
		mesh.texcoords.emplace_back(u, 0.0f);
		mesh.texcoords.emplace_back(u, 1.0f);
		const glm::vec3 n = glm::normalize(glm::vec3(x, 0.0f, z));
		mesh.normals.emplace_back(n);
		mesh.normals.emplace_back(n);

		const uint xid = 2*i;
		mesh.indices.push_back(xid);
		mesh.indices.push_back((xid+3)%(2*resolution));
		mesh.indices.push_back(xid+1);
		mesh.indices.push_back(xid);
		mesh.indices.push_back((xid+2)%(2*resolution));
		mesh.indices.push_back((xid+3)%(2*resolution));
	}
	return mesh;
}
