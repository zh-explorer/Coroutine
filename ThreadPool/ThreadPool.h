//
// Created by explorer on 2020/11/25.
//

#ifndef COROUTINE_THREADPOOL_H
#define COROUTINE_THREADPOOL_H

#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>

enum ExecuteStatus {
    padding,
    running,
    finished,
    canceled
};

class ThreadExecutor {
public:
    virtual void exec() = 0;

    int cancel();

    bool is_padding() {
        std::unique_lock<std::mutex> lk(m);
        return this->status == padding;
    }

    bool is_cancel() {
        std::unique_lock<std::mutex> lk(m);
        return this->status == canceled;
    }

    bool is_finish() {
        std::unique_lock<std::mutex> lk(m);
        return this->status == finished || this->status == canceled;
    }

    enum ExecuteStatus get_status() {
        std::unique_lock<std::mutex> lk(m);
        return this->status;
    }

private:
    std::mutex m;

    friend class ThreadWorker;

    void set_status(enum ExecuteStatus s) {
        std::unique_lock<std::mutex> lk(m);
        this->status = s;
    }

    enum ExecuteStatus status = padding;
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

    bool is_available() {
        std::unique_lock<std::mutex> lk(m);
        //a not busy and not stop thread is available
        return !this->busy || !this->thread_stop;
    }

    // get and clean the done executor
    ThreadExecutor *get_result() {
        std::unique_lock<std::mutex> lk(m);
        auto t = this->done_executor;
        this->done_executor = nullptr;
        return t;
    }

    ~ThreadWorker();

private:

    void task_done();

    pthread_t main_thread_id;
    pthread_t thread_id{};
    std::mutex m;
    bool thread_stop = false;
    bool busy = false;
    ThreadExecutor *executor = nullptr;
    ThreadExecutor *done_executor = nullptr;
    std::condition_variable cv;
};

#endif //COROUTINE_THREADPOOL_H
