//
// Created by explorer on 2020/12/1.
//

#ifndef COROUTINE_THREADEXECUTOR_H
#define COROUTINE_THREADEXECUTOR_H

#include <mutex>

class ThreadPool;

enum ThreadStatus {
    padding,
    running,
    finished,
    canceled
};

class ThreadExecutor {
public:
    virtual void exec() = 0;

    void cancel();

    bool is_running();

    bool is_padding();

    bool is_cancel();

    bool is_finish();

    bool is_success();

    // it's not good to handle a status with no lock
    enum ThreadStatus get_status() = delete;

private:
    std::mutex m;

    friend class ThreadWorker;

    friend class ThreadPool;

    ThreadPool *pool = nullptr;

    void set_status(enum ThreadStatus s);

    void set_pool(ThreadPool *p);

    enum ThreadStatus status = padding;
};

#endif //COROUTINE_THREADEXECUTOR_H
