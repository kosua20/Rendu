#include "resources/Bounds.hpp"

BoundingSphere::BoundingSphere(const glm::vec3 & aCenter, float aRadius) :
	center(aCenter), radius(aRadius) {
}

BoundingBox::BoundingBox(const glm::vec3 & v0, const glm::vec3 & v1) {
	minis = glm::min(v0, v1);
	maxis = glm::max(v0, v1);
}

BoundingBox::BoundingBox(const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2) {
	minis = glm::min(glm::min(v0, v1), v2);
	maxis = glm::max(glm::max(v0, v1), v2);
}

void BoundingBox::merge(const BoundingBox & box) {
	minis = glm::min(minis, box.minis);
	maxis = glm::max(maxis, box.maxis);
}

void BoundingBox::merge(const glm::vec3 & point) {
	minis = glm::min(minis, point);
	maxis = glm::max(maxis, point);
}

BoundingSphere BoundingBox::getSphere() const {
	const glm::vec3 center = 0.5f * (minis + maxis);
	const float radius	 = glm::length(maxis - center);
	return {center, radius};
}

glm::vec3 BoundingBox::getSize() const {
	return maxis - minis;
}

std::vector<glm::vec3> BoundingBox::getCorners() const {
	return {
		glm::vec3(minis[0], minis[1], minis[2]),
		glm::vec3(minis[0], minis[1], maxis[2]),
		glm::vec3(minis[0], maxis[1], minis[2]),
		glm::vec3(minis[0], maxis[1], maxis[2]),
		glm::vec3(maxis[0], minis[1], minis[2]),
		glm::vec3(maxis[0], minis[1], maxis[2]),
		glm::vec3(maxis[0], maxis[1], minis[2]),
		glm::vec3(maxis[0], maxis[1], maxis[2])};
}

std::vector<glm::vec4> BoundingBox::getHomogeneousCorners() const {
	return {
		glm::vec4(minis[0], minis[1], minis[2], 1.0f),
		glm::vec4(minis[0], minis[1], maxis[2], 1.0f),
		glm::vec4(minis[0], maxis[1], minis[2], 1.0f),
		glm::vec4(minis[0], maxis[1], maxis[2], 1.0f),
		glm::vec4(maxis[0], minis[1], minis[2], 1.0f),
		glm::vec4(maxis[0], minis[1], maxis[2], 1.0f),
		glm::vec4(maxis[0], maxis[1], minis[2], 1.0f),
		glm::vec4(maxis[0], maxis[1], maxis[2], 1.0f)};
}

glm::vec3 BoundingBox::getCentroid() const {
	return 0.5f * (minis + maxis);
}

BoundingBox BoundingBox::transformed(const glm::mat4 & trans) const {
	BoundingBox newBox;
	const std::vector<glm::vec4> corners = getHomogeneousCorners();

	newBox.minis						 = glm::vec3(trans * corners[0]);
	newBox.maxis						 = newBox.minis;
	for(size_t i = 1; i < 8; ++i) {
		const glm::vec3 transformedCorner = glm::vec3(trans * corners[i]);
		newBox.minis					  = glm::min(newBox.minis, transformedCorner);
		newBox.maxis					  = glm::max(newBox.maxis, transformedCorner);
	}
	return newBox;
}

bool BoundingBox::contains(const glm::vec3 & point) const {
	return glm::all(glm::greaterThanEqual(point, minis)) && glm::all(glm::lessThanEqual(point, maxis));
}

bool BoundingBox::empty() const {
	// Use the first component of minis as a canary.
	return minis[0] == std::numeric_limits<float>::max();
}

Frustum::Frustum(const glm::mat4 & vp){
	// We have to access rows easily, so transpose.
	const glm::mat4 tvp = glm::transpose(vp);
	const glm::mat4 ivp = glm::inverse(vp);
	// Based on Fast Extraction of Viewing Frustum Planes from the World- View-Projection Matrix, G. Gribb, K. Hartmann
	// (https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf)
	_planes[LEFT]   = tvp[3] + tvp[0];
	_planes[RIGHT]  = tvp[3] - tvp[0];
	_planes[TOP]    = tvp[3] - tvp[1];
	_planes[BOTTOM] = tvp[3] + tvp[1];
	_planes[NEAR]   = tvp[2];
	_planes[FAR]    = tvp[3] - tvp[2];

	// Reproject the 8 corners of the frustum from NDC to world space.
	static const std::array<glm::vec4, 8> ndcCorner = {
		glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
		glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
		glm::vec4(-1.0f,  1.0f, 1.0f, 1.0f),
		glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f),
		glm::vec4( 1.0f, -1.0f, 1.0f, 1.0f),
		glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		glm::vec4( 1.0f,  1.0f, 1.0f, 1.0f)};

	for(uint i = 0; i < 8; ++i){
		const glm::vec4 corn = ivp * ndcCorner[i];
		_corners[i] = glm::vec3(corn) / corn[3];
	}
}

bool Frustum::intersects(const BoundingBox & box) const {
	const std::vector<glm::vec4> corners = box.getHomogeneousCorners();
	// For each of the frustum planes, check if all points are in the "outside" half-space.
	for(uint pid = 0; pid < FrustumPlane::COUNT; ++pid){
		if((glm::dot(_planes[pid], corners[0]) < 0.0) &&
		   (glm::dot(_planes[pid], corners[1]) < 0.0) &&
		   (glm::dot(_planes[pid], corners[2]) < 0.0) &&
		   (glm::dot(_planes[pid], corners[3]) < 0.0) &&
		   (glm::dot(_planes[pid], corners[4]) < 0.0) &&
		   (glm::dot(_planes[pid], corners[5]) < 0.0) &&
		   (glm::dot(_planes[pid], corners[6]) < 0.0) &&
		   (glm::dot(_planes[pid], corners[7]) < 0.0)){
			return false;
		}
	}
	/// \todo Implement frustum corner checks to weed out more false positives.
	return true;
}

glm::mat4 Frustum::perspective(float fov, float ratio, float near, float far){
	glm::mat4 projection = glm::perspective(fov, ratio, near, far);
	projection[0][1] *= -1.0f;
	projection[1][1] *= -1.0f;
	projection[2][1] *= -1.0f;
	projection[3][1] *= -1.0f;
	return projection;
}

glm::mat4 Frustum::ortho(float left, float right, float bottom, float top, float near, float far){
	glm::mat4 projection = glm::ortho(left, right, bottom, top, near, far);
	projection[0][1] *= -1.0f;
	projection[1][1] *= -1.0f;
	projection[2][1] *= -1.0f;
	projection[3][1] *= -1.0f;
	return projection;
}
