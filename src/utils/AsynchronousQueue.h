/*
 * AsynchronousQueue.h
 *
 *  Created on: May 10, 2016
 *      Author: jamil
 */

#ifndef ASYNCHRONOUSQUEUE_H_
#define ASYNCHRONOUSQUEUE_H_

#include <deque>
#include <mutex>

template <typename T>
class AsynchronousQueue:public std::deque<T> {
	std::mutex q_mutex;
public:
	AsynchronousQueue();
	virtual ~AsynchronousQueue();

	void push_back(T &data) {
		q_mutex.lock();
		push_back(data);
		q_mutex.unlock();
	}

	T& pop_front();
};

#endif /* ASYNCHRONOUSQUEUE_H_ */
