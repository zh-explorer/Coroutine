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

    bool run(Executor *executor);

    void force_stop();

    void running();

    bool is_busy();

    bool stop();

    pthread_t get_thread_id() {
        return thread_id;
    }

private:
    Executor *executor = NULL;

    bool mark_stop = false;

    bool is_running = false;

    std::mutex m;

    std::condition_variable cv;

    pthread_t thread_id = 0;
};

// TODO use lambda
void thread_caller(aThread *thread);

#endif //COROUTINE_ATHREAD_H
