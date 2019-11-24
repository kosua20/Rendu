#pragma once

#include "system/Codable.hpp"
#include "Common.hpp"

/**
	\brief Wraps an animated property so that the initial value is preserved.
	\ingroup Scene
*/
template <typename T>
class Animated {
public:
	
	/** Constructor.
	 \param value the initial value
	 */
	explicit Animated(const T & value){
		_initial = value;
		_current = value;
	}
	
	/** Get the initial value.
	 \return the saved initial value
	 */
	const T & initial() const {
		return _initial;
	}
	
	/** Set the initial value.
	 \param value the initial value
	 */
	void reset(const T & value){
		_initial = value;
		_current = value;
	}
	
	/** Copy assignment operator for the current value.
	 \param value the new current value
	 \return a reference to the current value
	 \note This will not update the initial value.
	 */
	T & operator=(const T & value){
		_current = value;
		return *this;
	}
	
	/** Get the current value.
	\return the current value
	*/
	const T & get() const {
		return _current;
	}
	
	/** Conversion operator for the current value.
	 \return a reference to the current value
	 \note This will not update the initial value.
	 */
	operator T & () {
		return _current;
	}
	
	/** Conversion operator for the current value.
	\return a reference the current value
	*/
	operator const T & () const {
		return _current;
	}
	
private:
	
	T _initial = T(); ///< Initial value.
	T _current = T(); ///< Current value.
};
