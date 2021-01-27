/**
* A thread-safe queue
* At any given moment, the queue can be accessed by multiple resources at once
* so we need to make sure that the queue is thread-safe
*/
#pragma once

#include "lmqtt_common.h"

namespace lmqtt {

template<typename T>
class ts_priority_queue {
public:
	ts_priority_queue() = default;

	ts_priority_queue(const ts_priority_queue<T>&) = delete;
	~ts_priority_queue() {
		clear();
	}

	bool empty() {
		std::scoped_lock lock(_mxq);
		return _pq.empty();
	}

	size_t size() {
		std::scoped_lock lock(_mxq);
		return _pq.size();
	}

	const T& top() {
		std::scoped_lock lock(_mxq);
		return _pq.top()
	}

	void push(const T& item) {
		std::scoped_lock lock(_mxq);

		_pq.push(item);

		std::unique_lock<std::mutex> ul(_muxBlocking);
		cv.notify_one();
	}

	void pop() {
		std::scoped_lock lock(_mxq);

		_pq.pop();

		std::unique_lock<std::mutex> ul(_muxBlocking);
		cv.notify_one();
	}

	void wait() {
		while (empty()) {
			std::unique_lock<std::mutex> ul{ muxBlocking };
			// waits without consuming cpu
			cv.wait(ul);
		}
	}

protected:
	std::mutex _mxq, _muxBlocking;
	std::priority_queue<T> _pq;
	std::condition_variable cv;
};

} // namespace lmqtt

