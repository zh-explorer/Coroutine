//
// Created by explorer on 2020/11/25.
//

#ifndef COROUTINE_THREADPOOL_H
#define COROUTINE_THREADPOOL_H

#include <vector>
#include <csignal>

class ThreadWorker;
class ThreadExecutor;

class ThreadPool {
public:
    // if max_workers is 0. set max to unlimited
    explicit ThreadPool(int max_workers = 10);

    void submit(ThreadExecutor *executor);

    void cancel(ThreadExecutor *executor);

    void schedule();

    bool empty() {
        return this->busy_workers.empty();
    }

    ~ThreadPool();

private:
    // check padding executor and do_schedule it

    size_t max_workers;
    std::vector<ThreadWorker *> busy_workers;
    std::vector<ThreadWorker *> padding_works;
    std::vector<ThreadExecutor *> padding_executor;
};


#endif //COROUTINE_THREADPOOL_H
