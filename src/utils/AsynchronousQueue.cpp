/*
 * AsynchronousQueue.cpp
 *
 *  Created on: May 10, 2016
 *      Author: jamil
 */

#include "AsynchronousQueue.h"

template <typename T>
AsynchronousQueue<T>::AsynchronousQueue() {


}

template <typename T>
AsynchronousQueue<T>::~AsynchronousQueue() {
	// TODO Auto-generated destructor stub
}

template <typename T>
T& AsynchronousQueue<T>::pop_front(){
		q_mutex.lock();
		T& temp = this->front();
		pop_front();
		q_mutex.unlock();
		return temp;
}
