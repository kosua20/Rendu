#pragma once
#include "Common.hpp"
#include <array>

/**
 \brief Represent the sphere of smallest radius containing a given object or region of space.
 \ingroup Resources
 */
class BoundingSphere {
public:
	/** Empty sphere constructor. */
	BoundingSphere() = default;

	/** Constructor
	 \param aCenter the center of the sphere
	 \param aRadius the radius of the sphere
	 */
	BoundingSphere(const glm::vec3 & aCenter, float aRadius);

	glm::vec3 center = glm::vec3(0.0f); ///< The sphere center.
	float radius	 = 0.0f;			///< The sphere radius.
};

/**
 \brief Represent the smallest axis-aligne box containing a given object or region of space.
 \ingroup Resources
 */
class BoundingBox {
public:
	/** Empty box constructor. */
	BoundingBox() = default;

	/** Corner-based box constructor.
	 \param v0 first corner
	 \param v1 second corner
	 */
	BoundingBox(const glm::vec3 & v0, const glm::vec3 & v1);

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

	/** Query the homogeneous positions of the eight corners of the box, in the following order (with \p m=mini, \p M=maxi):
	 \p (m,m,m,1), \p (m,m,M,1), \p (m,M,m,1), \p (m,M,M,1), \p (M,m,m,1), \p (M,m,M,1), \p (M,M,m,1), \p (M,M,M,1)
	 \return a vector containing the box corners
	 */
	std::vector<glm::vec4> getHomogeneousCorners() const;

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

	/** \return true if no point has been added to the bounding box */
	bool empty() const;

	glm::vec3 minis = glm::vec3(std::numeric_limits<float>::max());	///< Lower-back-left corner of the box.
	glm::vec3 maxis = glm::vec3(std::numeric_limits<float>::lowest()); ///< Higher-top-right corner of the box.
};


/** \brief Represent a 3D frustum, volume defined by the intersection of six planes.
  \ingroup Resources
 */
class Frustum {
public:

	/**Create a frustum from a view-projection matrix.
	 \param vp the matrix defining the frustum
	 */
	Frustum(const glm::mat4 & vp);

	/** Indicate if a bounding box intersect this frustum.
	\param box the bounding box to test
	\return true if the bounding box intersects the frustum.
	*/
	bool intersects(const BoundingBox & box) const;


	static glm::mat4 perspective(float fov, float ratio, float near, float far);

private:

	/** Helper enum for the frustum plane locations. */
	enum FrustumPlane : uint {
		LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, NEAR = 4, FAR = 5, COUNT = 6
	};

	std::array<glm::vec4, FrustumPlane::COUNT> _planes; ///< Frustum hyperplane coefficients.
	std::array<glm::vec3, 8> _corners; ///< Frustum corners.
};
