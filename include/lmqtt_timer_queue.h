#pragma once

//#include "lmqtt_tspqueue.h"
#include "lmqtt_common.h"

namespace lmqtt {

class timer_queue {
public:
    struct comparator {
        bool operator() (const std::chrono::milliseconds& l,
            const std::chrono::milliseconds& r) {

            size_t ltime = std::chrono::duration_cast<std::chrono::milliseconds>(l).count();
            size_t rtime = std::chrono::duration_cast<std::chrono::milliseconds>(r).count();

            return ltime < rtime;
        }

        bool operator() (size_t ltime, size_t rtime) {
            return ltime < rtime;
        }
    };

    void run() {
        while (!_exit) {
            while (_queue.empty());

            size_t time = _queue.top();
            _queue.pop();

            std::cout << "Thread " << std::this_thread::get_id() << " will sleep for " << time << " ms\n";

            std::this_thread::sleep_for(time);
        }
    }

    timer_queue() {

    }

    ~timer_queue() = default;

private:

    std::priority_queue<size_t, std::vector<size_t>, comparator> _queue;
    std::thread _worker;
    bool _exit = false;

};

} // namespace lmqtt