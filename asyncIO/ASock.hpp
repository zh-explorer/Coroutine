//
// Created by explorer on 2020/12/17.
//

#ifndef COROUTINE_ASOCK_HPP
#define COROUTINE_ASOCK_HPP

#include "sock.h"
#include "../async/Event.h"

class ASock : public Event, public Sock {
public:

    explicit ASock(int fd) : Sock(fd) {};

    void wait_read(int timeout = -1) {
        this->can_read = false;
        int start_time = time(nullptr);
        int t = timeout;
        while (this->can_read || this->fin) {
            current_loop->wait_event(this, t);
            if (timeout != -1) {
                t = timeout - (time(nullptr) - start_time);
                if (t <= 0) {
                    return;
                }
            }
        }
    }

    void wait_write(int timeout = -1) {
        this->can_write = false;
        int start_time = time(nullptr);
        int t = timeout;
        while (this->can_write || this->fin) {
            current_loop->wait_event(this, t);
            if (timeout != -1) {
                t = timeout - (time(nullptr) - start_time);
                if (t <= 0) {
                    return;
                }
            }
        }
    }

    bool event_set() override {
        return this->can_read || this->can_write || this->fin;
    }

private:
};

#endif //COROUTINE_ASOCK_HPP
