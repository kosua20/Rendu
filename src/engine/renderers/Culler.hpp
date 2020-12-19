#pragma once

#include "Common.hpp"
#include "scene/Object.hpp"

/**
 \brief
 \details
 \ingroup Renderers
 */
class Culler {

public:

	/** Constructor
	 \param objects the list of objects to process
	 */
	Culler(const std::vector<Object> & objects);

	/** Detect objects that are inside the view frustum. This returns the indices of the objects that are visible in a list padded to the objects count with -1s.
	 \param view the view matrix
	 \param proj the projection matrix
	 \return indices of the objects to be rendered
	 \note As soon as a -1 is encountered in the list, all further indices will also be -1.
	 */
	const std::vector<long> & cull(const glm::mat4 & view, const glm::mat4 & proj);

	/** Detect objects that are inside the view frustum and sort them based on their type. This returns the object indices in a list padded to the objects count with -1s.
	 \param view the view matrix
	 \param proj the projection matrix
	 \return indices of the objects to be rendered
	 \note As soon as a -1 is encountered in the list, all further indices will also be -1.
	 */
	const std::vector<long> & cullAndSort(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos);

	/** Display culling options GUI. */
	void interface();

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Culler & operator=(const Culler &) = delete;
	
	/** Copy constructor (disabled). */
	Culler(const Culler &) = delete;
	
	/** Move assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Culler & operator=(Culler &&) = delete;
	
	/** Move constructor (disabled). */
	Culler(Culler &&) = delete;

private:

	const std::vector<Object> & _objects; ///< Reference to the objects to process.
	std::vector<long> _order; ///< Will contain the indices of the objects selected.

	/** Information for object sorting. */
	struct DistPair {
		long id = -1; ///< Index of the object.
		double distance = std::numeric_limits<double>::max(); ///< Distance.
	};
	std::vector<DistPair> _distances; ///< Intermediate storage for sorting.

	Frustum _frustum; ///< Current view frustum.
	unsigned long _maxCount; ///< Maximum number of objects to select
	bool _freezeFrustum = false; ///< Should the frustum not be updated.

};
