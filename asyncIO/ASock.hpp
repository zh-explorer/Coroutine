//
// Created by explorer on 2020/12/17.
//

#ifndef COROUTINE_ASOCK_HPP
#define COROUTINE_ASOCK_HPP

#include "sock.h"
#include "Event.h"
#include "async.h"
#include <ctime>

class ASock : public Event, public Sock {
public:

    explicit ASock(int fd) : Sock(fd) {};

    void wait_read(int timeout = -1) {
        this->can_read = false;
        this->need_read = true;

        int start_time = time(nullptr);
        int t = timeout;
        while (!this->can_read && !this->fin) {
            current_loop->wait_event(this, t);
            if (timeout != -1) {
                t = timeout - (time(nullptr) - start_time);
                if (t <= 0) {
                    this->need_read = false;
                    return;
                }
            }
        }
        this->need_read = false;
    }

    void wait_write(int timeout = -1) {
        this->can_write = false;
        this->need_write = true;

        int start_time = time(nullptr);
        int t = timeout;
        while (!this->can_write && !this->fin) {
            current_loop->wait_event(this, t);
            if (timeout != -1) {
                t = timeout - (time(nullptr) - start_time);
                if (t <= 0) {
                    this->need_write = false;
                    return;
                }
            }
        }
        this->need_write = false;
    }

    bool event_set() override {
        if (this->fin) {
            return true;
        }
        if (this->need_write && this->can_write) {
            return true;
        }
        if (this->can_read && this->need_read) {
            return true;
        }
    }

private:
    bool need_read;
    bool need_write;
};

#endif //COROUTINE_ASOCK_HPP
