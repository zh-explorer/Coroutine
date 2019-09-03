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

    virtual  ~aThread();

    bool run(Executor *executor);

    void running();

    bool is_busy();

    bool stop();

private:
    Executor *executor = NULL;

    bool mark_stop = false;

    bool is_running = false;

    std::mutex m;

    std::condition_variable cv;

    std::thread thread;

    pthread_t thread_id = 0;
};


void thread_caller(aThread *thread);

#endif //COROUTINE_ATHREAD_H
