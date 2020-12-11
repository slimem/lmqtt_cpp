/**
* A thread-safe queue
* At any given moment, the queue can be accessed by multiple resources at once
* so we need to make sure that the queue is thread-safe
*/
#pragma once

#include "lmqtt_common.h"

namespace lmqtt {

	template<typename T>
	class ts_queue {
	public:
		ts_queue() = default;
		// we cannot allow our ts_queue to be copied because it has
		// mutexes in it
		ts_queue(const ts_queue<T>&) = delete;
		~ts_queue() {
			clear();
		}
	
		const T& front() {
			// always use scoped_lock (C++17) for mutexes because it supports 
			// multiple mutextes at the same time with lock free algorithm
			std::scoped_lock lock(_mxq);
			return _deq.front();
		}
	
		void push_front(const T& item) {
			std::scoped_lock lock(_mxq);
			// use emplace_ to avoid unnecessary copy
			_deq.push_front(std::move(item));

			std::unique_lock<std::mutex> lock(muxBlocking);
			cv.notify_one();
		}
	
		// to actually get the item at the front
		[[nodiscard]] T pop_front() {
			std::scoped_lock lock(_mxq);
			auto item = std::move(_deq.front());
			_deq.pop_front();
			return item;
		}

		// check if it's needed
		/*T operator [](int i) const {
			std::scoped_lock(_mxq);
			return _deq[i];
		}*/

		T& operator [](int i) {
			std::scoped_lock(_mxq);
			return _deq[i];
		}
	
		const T& back() {
			std::scoped_lock(_mxq);
			return _deq.back();
		}
	
		void push_back(const T& item) {
			std::scoped_lock lock(_mxq);
			_deq.push_back(std::move(item));

			std::unique_lock<std::mutex> lock(muxBlocking);
			cv.notify_one();
		}
	
		[[nodiscard]] T pop_back() {
			std::scoped_lock lock(_mxq);
			auto item = std::move(_deq.back());
			_deq.pop_back();
			return item;
		}
	
		bool empty() {
			std::scoped_lock lock(_mxq);
			return _deq.empty();
		}
	
		size_t size() {
			std::scoped_lock lock(_mxq);
			return _deq.size();
		}
	
		void clear() {
			std::scoped_lock lock(_mxq);
			_deq.clear();
		}

		void wait() {
			while (empty()) {
				std::unique_lock<std::mutex> ul(muxBlocking);
				// waits without consuming cpu
				cv.wait(ul);
			}
		}
	
	protected:
		std::mutex _mxq, muxBlocking;
		std::deque<T> _deq;
		std::condition_variable cv;
	};

} // namespace lmqtt

