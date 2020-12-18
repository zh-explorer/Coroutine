//
// Created by explorer on 2020/12/17.
//

#include "AIO.h"
#include <cstring>
#include "unistd.h"

// return the accurate size read.
// if read size not equal size arg
// 1. timeout
// 2. timeout is 0 and no enough data
// 3. error or close while read but get some data
// if ret a -1, this mean the socket is close and can not read data anymore
int AIO::read(unsigned char *buffer, size_t size, int timeout) {
    int read_size = 0;
    int result;
    bool in_pool = false;
    int start_time = time(nullptr);

    while (read_size != size) {
        result = ::read(this->fd, buffer + read_size, size - read_size);

        if (result == -1 && errno != EAGAIN) {
            // get a error while read data
            logger(INFO, stderr, "error while read socket fd: %d, err: %s", this->fd, strerror(errno));
//            if (read_size == 0) {
//                // this mean that socket is close or error before read. so return -1
//                return -1;
//            } else {
//                // we read some data and get error, return the size we read;
//                return read_size;
//            }
            return read_size == 0 ? -1 : read_size;
        } else if (result == 0 || (result == -1 && errno == EAGAIN)) {
            // no data to read
            if (timeout == 0) {
                return read_size;
            } else {
                if (!in_pool) {
                    auto poll = current_loop->get_epoll();
                    poll->add_read(this->aSock);
                    in_pool = true;
                }

                // calc timeout
                int t;
                if (timeout == -1) {
                    t = -1;
                } else {
                    t = timeout - (time(nullptr) - start_time);
                    if (t <= 0) {
                        return read_size; // timeout
                    }
                }

                aSock->wait_read(t);
                // this will return while timeout, the fd can not be read in this time
                // but we don't handle it in this place.
                // the read next time will failed and timeout will be check
                if (aSock->is_fin()) {
                    // the sock is fin, is read_size == 0, ret -1 to notify
                    return read_size == 0 ? -1 : read_size;
                }
                // we not read data in last read, so set result to 0 and continue
                result = 0;
            }
        }
        read_size += result;
    }
    if (in_pool) {
        auto poll = current_loop->get_epoll();
        poll->delete_read(this->aSock);
    }
    return read_size;
}

int AIO::write(unsigned char *buffer, size_t size, int timeout) {
    int write_size = 0;
    int result;
    bool in_pool = false;
    int start_time = time(nullptr);

    while (write_size != size) {
        result = ::write(this->fd, buffer + write_size, size - write_size);

        if (result == -1 && errno != EAGAIN) {
            logger(INFO, stderr, "error while write socket fd: %d, err: %s", this->fd, strerror(errno));
            return write_size == 0 ? -1 : write_size;
        } else if (result == 0 || (result == -1 && errno == EAGAIN)) {
            if (timeout == 0) {
                return write_size;
            } else {
                if (!in_pool) {
                    auto poll = current_loop->get_epoll();
                    poll->add_write(this->aSock);
                    in_pool = true;
                }

                int t;
                if (timeout == -1) {
                    t = -1;
                } else {
                    t = timeout - (time(nullptr) - start_time);
                    if (t <= 0) {
                        return write_size;
                    }
                }

                aSock->wait_write(t);
                if (aSock->is_fin()) {
                    return write_size == 0 ? -1 : write_size;
                }
                result = 0;
            }
        }

        write_size += result;
    }
    if (in_pool) {
        auto poll = current_loop->get_epoll();
        poll->delete_write(this->aSock);
    }
    return write_size;
}
