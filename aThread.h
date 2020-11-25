//
// Created by explorer on 19-8-31.
//

#ifndef COROUTINE_ATHREAD_H
#define COROUTINE_ATHREAD_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class Executor;

// aThread is not a thead save class.
class aThread {
public:
    explicit aThread();

    aThread(aThread &) = delete;

    aThread(aThread &&) = delete;

    bool start(Executor *executor);

    void force_stop();

    void run();

    bool is_busy();

    bool stop();

    pthread_t get_thread_id() const {
        return thread_id;
    }

private:
    Executor *executor = nullptr;

    bool thread_stop = false;

    bool running = false;

    std::mutex m;

    std::condition_variable cv;

    pthread_t thread_id = 0;
};

#endif //COROUTINE_ATHREAD_H
