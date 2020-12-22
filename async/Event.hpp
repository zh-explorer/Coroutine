//
// Created by explorer on 2020/12/9.
//
#ifndef COROUTINE_EVENT_HPP
#define COROUTINE_EVENT_HPP
#include "Event.h"
#include "../unit/log.h"


// some simple event
template<typename T>
class Future : public Event {
    void set(T v) {
        this->value = v;
        this->flag = true;
    }

    bool is_set() {
        return this->flag;
    }

    void clear() {
        this->flag = false;
    }

    T get_val() {
        return value;
    }

    bool event_set() override {
        return this->is_set();

    }

    void wait(int timeout = -1) {
        current_loop->wait_event(this, timeout);
    }

private:
    bool flag = false;
    T value{};
};

template<>
class Future<void> : public Event {
public:
    void set() {
        this->flag = true;
    }

    bool is_set() const {
        return this->flag;
    }

    void clear() {
        this->flag = false;
    }

    bool event_set() override {
        return this->is_set();

    }

    bool wait(int timeout = -1) {
        current_loop->wait_event(this, timeout);
        return this->is_set();
    }

private:
    bool flag = false;
};

class Sleep : public Event {
public:
    void sleep(int second) {
        if (second <= 0) {
            return;
        }
        current_loop->wait_event(this, second);
    }

    void sleep_to(int timestamp) {
        this->sleep(timestamp - time(nullptr));
    }

    bool event_set() override {
        return false;
    }
};

class Mutex {
public:
    Mutex(Mutex &m) = delete;

    Mutex(Mutex &&m) = delete;

    explicit Mutex(bool m = false) {
        this->m = m;
    }

    bool set() {
        if (this->m) {
            return false;
        }
        this->m = true;
        return true;
    }

    bool clear() {
        this->m = false;
    }

    bool is_set() const {
        return this->m;
    }

private:
    bool m;
};

class Lock : public Event {
public:
    explicit Lock(bool locked = false) : _m(locked) {
        this->m = &_m;
    }

    explicit Lock(Mutex *m) {
        this->m = m;
    }

    explicit Lock(Mutex &m) {
        this->m = __builtin_addressof(m);
    }

    bool acquire(bool blocking = true, int timeout = -1) {
        if (!this->m->is_set()) {
            this->m->set();
            return true;
        }
        if (!blocking) {
            return false;
        }

        current_loop->wait_event(this, timeout);

        if (!this->m->is_set()) {
            this->m->set();
            return true;
        } else {
            return false;

        }
    }

    void release() {
        this->m->clear();
    }

    bool event_set() override {
        return !this->m->set();
    }

    bool is_locked() {
        return this->m->set();
    }

protected:
    Mutex *m;
    Mutex _m{};
};

class UniqueLock : public Lock {
public:
    explicit UniqueLock(Mutex *m) : Lock(m) {
        auto result = this->acquire();
        assert(result);
    }

    explicit UniqueLock(Mutex &m) : Lock(m) {
        auto result = this->acquire();
        assert(result);
    }

    ~UniqueLock() {
        this->m->clear();
    }
};

class Condition {
public:
    Condition(Condition &) = delete;

    void wait(Lock *lock) {
        this->wait(lock, -1);
    }

    void wait(Lock *lock, std::function<bool()> f) {
        while (!f()) {
            this->wait(lock, -1);
        }
    }

    void wait_for(Lock *lock, int timeout) {
        this->wait(lock, timeout);
    }

    void wait_until(Lock *lock, int timeout_time) {
        int timeout = timeout_time - time(nullptr);
        if (timeout < 0) {
            timeout = 0;
        }
        this->wait(lock, timeout);
    }

    void notify(unsigned int n) {
        auto iter = this->event_list.begin();
        for (int i = 0; i < n && iter != this->event_list.end(); i++) {
            (*iter)->set();
            iter = this->event_list.erase(iter);
        }
    }

    void notify_all() {
        for (auto f:this->event_list) {
            f->set();
        }
        this->event_list.clear();
    }

    void notify_one() {
        this->notify(1);
    }

private:
    std::vector<Future<void> *> event_list;

    void wait(Lock *lock, int timeout) {
        lock->release();

        auto f = new Future<void>;
        this->event_list.push_back(f);
        f->wait(timeout);

        lock->acquire();
    }
};

class Semaphore : Event {
public:
    explicit Semaphore(unsigned value = 1) {
        this->lock_value = value;
    }

    void acquire() {
        while (this->lock_value == 0) {
            current_loop->wait_event(this, -1);
        }
        this->lock_value--;
    }

    bool event_set() override {
        return this->lock_value != 0;
    }

    bool locked() const {
        return this->lock_value == 0;
    }

    void release() {
        this->lock_value++;
    }

private:
    unsigned int lock_value;
};
#endif //COROUTINE_EVENT_HPP