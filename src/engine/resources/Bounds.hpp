#pragma once
#include "Common.hpp"

/**
 \brief Represent the sphere of smallest radius containing a given object or region of space.
 \ingroup Resources
 */
struct BoundingSphere {
	
	glm::vec3 center; ///< The sphere center.
	float radius; ///< The sphere radius.
	
	/** Empty sphere constructor. */
	BoundingSphere();
	
	/** Constructor
	 \param aCenter the center of the sphere
	 \param aRadius the radius of the sphere
	 */
	BoundingSphere(const glm::vec3 & aCenter, const float aRadius);
};

/**
 \brief Represent the smallest axis-aligne box containing a given object or region of space.
 \ingroup Resources
 */
struct BoundingBox {
	
	glm::vec3 minis; ///< Lower-back-left corner of the box.
	glm::vec3 maxis; ///< Higher-top-right corner of the box.
	
	/** Empty box constructor. */
	BoundingBox();
	
	/** Triangle-based box constructor.
	 \param v0 first triangle vertex
	 \param v1 second triangle vertex
	 \param v2 third triangle vertex
	 */
	BoundingBox(const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2);
	
	/** Extends the current box by another one. The result is the bounding box of the two boxes union.
	 \param box the bounding box to include
	 */
	void merge(const BoundingBox & box);
	
	/** Extends the current box by a point
	 \param point the point to include
	 */
	void merge(const glm::vec3 & point);
	
	/** Query the bounding sphere of this box.
	 \return the bounding sphere
	 */
	BoundingSphere getSphere() const;
	
	/** Query the size of this box.
	 \return the size
	 */
	glm::vec3 getSize() const;
	
	/** Query the positions of the eight corners of the box, in the following order (with \p m=mini, \p M=maxi):
	 \p (m,m,m), \p (m,m,M), \p (m,M,m), \p (m,M,M), \p (M,m,m), \p (M,m,M), \p (M,M,m), \p (M,M,M)
	 \return a vector containing the box corners
	 */
	std::vector<glm::vec3> getCorners() const;
	
	/** Query the center of the bounding box.
	 \return the centroid
	 */
	glm::vec3 getCentroid() const;
	
	/** Compute the bounding box of the transformed current box.
	 \param trans the transformation to apply
	 \return the bounding box of the transformed box
	 */
	BoundingBox transformed(const glm::mat4 & trans) const;
	
	/** Indicates if a point is inside the bounding box.
	 \param point the point to check
	 \return true if the bounding box contains the point
	 */
	bool contains(const glm::vec3 & point) const;
};

