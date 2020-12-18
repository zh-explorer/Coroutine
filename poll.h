//
// Created by explorer on 8/25/19.
//

#ifndef COROUTINE_POLL_H
#define COROUTINE_POLL_H

#include <cstdint>
#include <sys/epoll.h>
#include <vector>
#include <map>
// In linux we call user Epoll for our poll
// Maybe user select or poll for windows/macos

enum poll_event {
    READ,
    WRITE,
    ERROR,
};

struct poll_result {
    int fd;
    enum poll_event event;
    void *data;

    int err_number;

    poll_result(int fd, enum poll_event event, void *data, int err_number = 0) {
        this->fd = fd;
        this->event = event;
        this->data = data;
        if (event == ERROR) {
            this->err_number = err_number;
        } else {
            this->err_number = 0;
        }
    }
};

// a poll use for aio
class poll {
public:
    virtual bool add_read(int fd, void *data) = 0;

    virtual bool add_write(int fd, void *data) = 0;

    virtual void *delete_read(int fd) = 0;

    virtual void *delete_write(int fd) = 0;

    virtual bool empty() = 0;

    virtual std::vector<poll_result> *wait_poll(int timeout) = 0;
};


class poll_ev {
public:
    explicit poll_ev(int fd);

    void add_read(void *data);

    void add_write(void *data);

    void *delete_read();

    void *delete_write();

    bool in_poll() {
        return events & EPOLLIN || events & EPOLLOUT;
    }

    struct epoll_event ev{};

    struct epoll_event *ret_ev() {
        ev.events = events;
        ev.data.ptr = this;
        return &ev;
    }

    int fd;
    void *read_data;
    void *write_data;
    uint32_t events;
};

class EPoll : public poll {
public:
    EPoll();

    bool add_read(int fd, void *data) override;

    bool add_write(int fd, void *data) override;

    void *delete_read(int fd) override;

    void *delete_write(int fd) override;

    bool empty() override {
        return fds.empty();
    }

    std::vector<poll_result> *wait_poll(int timeout) override;

private:
    std::map<int, poll_ev> fds;
    int epoll_fd;
};


#endif //COROUTINE_POLL_H