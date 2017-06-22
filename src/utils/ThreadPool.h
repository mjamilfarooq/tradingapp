/*
 * ThreadPool.h
 *
 *  Created on: May 10, 2016
 *      Author: jamil
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>


template <class T>
class ThreadPool {
	//mantaining queue of pending tasks
	std::queue<T> tasks_queue;
	std::mutex tasks_queue_mutex;
	std::condition_variable queue_size_cond;


	std::mutex pool_mutex;
	size_t pool_size;

	std::vector<std::thread> threads;
	bool running;

	std::function<void (T)> thread_func;
	std::atomic_uint running_tasks;

	bool stop;

	void run() {
//		std::cout<<threads.size()<<std::endl;
		//this should run all threads in thread pool all at once
		for (auto& t: threads) {
			t = std::thread(&ThreadPool::loop, this);
		}
	}

	void loop() {
		std::cout<<"entering "<<std::this_thread::get_id()<<std::endl;
		while ( !stop ) {
			//check if queue has task to process
			std::unique_lock<std::mutex> lock(tasks_queue_mutex);
			queue_size_cond.wait(lock,
					[this]() {
						return this->tasks_queue.size() > 0 || stop ;
					}
			);

			if ( stop ) break;

			running_tasks++;
			T task = tasks_queue.front();
			tasks_queue.pop();
			lock.unlock();

			//run the thread pool function
//			std::cout<<"thread id "<<std::this_thread::get_id()<<" ";
			thread_func(task);
			running_tasks--;
			std::this_thread::sleep_for(std::chrono::seconds(2));

		}

		std::cout<<"exiting "<<std::this_thread::get_id()<<std::endl;
	}

public:

	ThreadPool(std::function <void (T)> func, size_t size):
		thread_func(func),
		pool_size(size),
		threads(size),
		stop(false) {
		this->run();
	}

	~ThreadPool() {
		stop_execution();
	}

	//push data in the pending tasks queue
	void push_task(T data) {

		{
//			std::cout<<"pushing data:lock"<<std::endl;
			std::lock_guard<std::mutex> lock(tasks_queue_mutex);
			tasks_queue.push(data);
//			std::cout<<"pushing data:unlock"<<std::endl;
		}

		queue_size_cond.notify_one();	//notify waiting thread of push op
	}

	//overloaded push task
	void operator()(T data) {
		push_task(data);
	}

	//pending tasks
	size_t tasks_pending() {
		size_t ret;
		{
			std::lock_guard<std::mutex> lock(tasks_queue_mutex);
			ret = tasks_queue.size();

		}
		return ret;
	}

	//running tasks
	size_t tasks_running() {
		return running_tasks.load();
	}

	//pool size
	size_t size() {
		return pool_size;
	}

	//stop task execution
	void stop_execution() {
		stop = true;
		queue_size_cond.notify_all();

		//join all the threads
		for (auto& t: threads) {
			t.join();
		}
	}

};

#endif /* THREADPOOL_H_ */
