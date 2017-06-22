/*
 * AsynchronousUnorderedMap.h
 *
 *  Created on: Jul 29, 2016
 *      Author: jamil
 */

#ifndef SRC_UTILS_ASYNCHRONOUSUNORDEREDMAP_H_
#define SRC_UTILS_ASYNCHRONOUSUNORDEREDMAP_H_

#include <unordered_map>
#include <utility>
#include <mutex>



template <typename Key, typename Value>
class AsynchronousUnorderedMap: public std::unordered_map<Key, Value> {
	std::mutex rw_mutex;
public:
	AsynchronousUnorderedMap() {

	}

	~AsynchronousUnorderedMap() {

	}

	using unordered_map = std::unordered_map<Key, Value>;
	using iterator = typename unordered_map::iterator;
	using value_type = typename unordered_map::value_type;
	using key_type = typename unordered_map::key_type;
	using size_type = typename unordered_map::size_type;
	using const_iterator = typename unordered_map::const_iterator;
	using mapped_type = typename unordered_map::mapped_type;



	template < typename... _Args >
	std::pair<iterator, bool>
	emplace(_Args&&... __args) {
		std::lock_guard<std::mutex> lock(rw_mutex);
		return unordered_map::emplace(std::forward<_Args>(__args)...);
	}

	std::pair<iterator, bool>
	insert(const value_type& __x) {
		std::lock_guard<std::mutex> lock(rw_mutex);
		return insert(__x);
	}

	template<typename _Pair, typename = typename std::enable_if<std::is_constructible<value_type, _Pair&&>::value>::type>
	std::pair<iterator, bool>
	insert(_Pair&& __x) {
		std::lock_guard<std::mutex> lock(rw_mutex);
		return unordered_map::insert(std::forward<_Pair>(__x));
	}

    iterator erase(const_iterator __position) {
    	std::lock_guard<std::mutex> lock(rw_mutex);
    	return unordered_map::erase(__position);
    }


    iterator erase(iterator __it) {
    	std::lock_guard<std::mutex> lock(rw_mutex);
    	return unordered_map::erase(__it);
    }

    size_type erase(const key_type& __x) {
    	std::lock_guard<std::mutex> lock(rw_mutex);
    	return unordered_map::erase(__x);
    }

    iterator erase(const_iterator __first, const_iterator __last) {
       	std::lock_guard<std::mutex> lock(rw_mutex);
    	return unordered_map::erase(__first, __last);
    }


    void clear() noexcept {
    	std::lock_guard<std::mutex> lock(rw_mutex);
    	unordered_map::clear();
    }


    iterator find(const key_type& __x) {
    	std::lock_guard<std::mutex> lock(rw_mutex);
    	return unordered_map::find(__x);
    }

     const_iterator find(const key_type& __x) const {
    	 std::lock_guard<std::mutex> lock(rw_mutex);
    	 return unordered_map::find(__x);
     }


     size_type count(const key_type& __x) const {
     	 std::lock_guard<std::mutex> lock(rw_mutex);
    	 return unordered_map::count(__x);
     }


     mapped_type& operator[](const key_type& __k) {
     	 std::lock_guard<std::mutex> lock(rw_mutex);
    	 return unordered_map::operator[](__k);
     }

     mapped_type& operator[](key_type&& __k) {
     	 std::lock_guard<std::mutex> lock(rw_mutex);
    	 return unordered_map::operator[](std::move(__k));
     }

	void lock() {
		rw_mutex.lock();
	}

	void unlock() {
		rw_mutex.unlock();
	}

};

#endif /* SRC_UTILS_ASYNCHRONOUSUNORDEREDMAP_H_ */
