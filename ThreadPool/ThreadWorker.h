//
// Created by explorer on 2020/11/30.
//

#ifndef COROUTINE_THREADWORKER_H
#define COROUTINE_THREADWORKER_H


#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "./ThreadExecutor.h"

enum WorkerStatus {
    waiting,
    working,
    finish,
    stopped
};

class ThreadWorker {
public:
    ThreadWorker();

    ThreadWorker(ThreadWorker &) = delete;

    ThreadWorker(ThreadWorker &&) = delete;

    void run();

    bool start(ThreadExecutor *new_executor);


    bool stop();

    void force_stop();

    // DO NOT CALL THIS IN CLASS METHOD BECAUSE OF DEADLOCK
    bool is_available() {
        std::unique_lock<std::mutex> lk(m);
        return this->status == waiting;
    }

    bool is_working() {
        std::unique_lock<std::mutex> lk(m);
        return this->status == working;
    }

    bool is_finish() {
        std::unique_lock<std::mutex> lk(m);
        return this->status == finish;
    }

    bool is_stop() {
        std::unique_lock<std::mutex> lk(m);
        return this->status == stopped;
    }

    bool executor_find(ThreadExecutor *e) {
        std::unique_lock<std::mutex> lk(m);
        return e == this->executor;
    }

    // not good to read status with no lock
    enum WorkerStatus get_status() = delete;

    ~ThreadWorker();

private:

    void task_done() const;

    pthread_t main_thread_id;
    pthread_t thread_id{};
    std::mutex m;
    enum WorkerStatus status = waiting;
    ThreadExecutor *executor = nullptr;
    std::condition_variable cv;
};

#endif //COROUTINE_THREADWORKER_H
