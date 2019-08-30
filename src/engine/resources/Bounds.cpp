#include "resources/Bounds.hpp"

BoundingSphere::BoundingSphere(const glm::vec3 & aCenter, float aRadius) :
	center(aCenter), radius(aRadius) {
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

glm::vec3 BoundingBox::getCentroid() const {
	return 0.5f * (minis + maxis);
}

BoundingBox BoundingBox::transformed(const glm::mat4 & trans) const {
	BoundingBox newBox;
	const std::vector<glm::vec3> corners = getCorners();
	newBox.minis						 = glm::vec3(trans * glm::vec4(corners[0], 1.0f));
	newBox.maxis						 = newBox.minis;
	for(size_t i = 0; i < 8; ++i) {
		const glm::vec3 transformedCorner = glm::vec3(trans * glm::vec4(corners[i], 1.0f));
		newBox.minis					  = glm::min(newBox.minis, transformedCorner);
		newBox.maxis					  = glm::max(newBox.maxis, transformedCorner);
	}
	return newBox;
}

bool BoundingBox::contains(const glm::vec3 & point) const {
	return glm::all(glm::greaterThanEqual(point, minis)) && glm::all(glm::lessThanEqual(point, maxis));
}
