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

            std::this_thread::sleep_for(std::chrono::milliseconds(time));
        }
    }

    timer_queue() {
        _queue.emplace(5000);
        _queue.emplace(3000);
        _queue.emplace(6000);
        _queue.emplace(9000);
    }

    ~timer_queue() {
        _worker.join();
    }

private:

    std::priority_queue<size_t, std::vector<size_t>, comparator> _queue;
    std::thread _worker{ [this]() { run(); } };
    bool _exit = false;

};

} // namespace lmqtt