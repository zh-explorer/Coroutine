//
// Created by explorer on 8/25/19.
//

#ifndef COROUTINE_AIO_H
#define COROUTINE_AIO_H

#include <sys/types.h>
#include <sys/socket.h>
#include "unit/array_buf.h"

enum READ_MODE {
    read_fix,  // read data until size reach count
    read_imm,  // return all data(less than count) immediately, return zero if no data can read immediately
    read_any, // try to read some data, is none, wait
};

enum WRITE_MODE {
    write_all,
    write_any,
};

class aio {
public:
    aio(int domain, int type, int protocol);

    explicit aio(int fd);

    int write(unsigned char *buf, size_t count, enum WRITE_MODE write_mode = write_all);

    int read(unsigned char *buf, size_t count, enum READ_MODE mode = read_fix);

    // get the cache data size.
    unsigned int cache_size() {
        return arrayBuf.length();
    }

    int bind(const struct sockaddr *addr, socklen_t addrlen);

    int bind(int port);

    int fileno;
private:
    array_buf arrayBuf;

    void set_noblock();
};


class aio_client : public aio {
    int connect(const struct sockaddr *addr, socklen_t addrlen);

    int connect(char *addr, int port);
};

class aio_server : public aio {

    int listen(int backlog);

    aio *accept();
};

#endif //COROUTINE_AIO_H
