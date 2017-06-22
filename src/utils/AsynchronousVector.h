/*
 * AsynchronousVector.h
 *
 *  Created on: Aug 29, 2016
 *      Author: jamil
 */

#ifndef SRC_UTILS_ASYNCHRONOUSVECTOR_H_
#define SRC_UTILS_ASYNCHRONOUSVECTOR_H_

#include <vector>
#include <mutex>

template <typename T>
class AsynchronousVector: public std::vector<T> {
	std::mutex rw_mutex;

public:

	using vector = std::vector<T>;
	using iterator = typename vector::iterator;
	using value_type = typename vector::value_type;
	using size_type = typename vector::size_type;
	using const_iterator = typename vector::const_iterator;
	using reference = typename vector::reference;
	using const_reference = typename vector::const_reference;


	void push_back(value_type &x) {
		std::lock_guard<std::mutex> lock(rw_mutex);
		vector::push_back(x);
	}

};



#endif /* SRC_UTILS_ASYNCHRONOUSVECTOR_H_ */
