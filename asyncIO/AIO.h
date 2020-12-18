//
// Created by explorer on 2020/12/17.
//

#ifndef COROUTINE_AIO_H
#define COROUTINE_AIO_H

#include "../async/Event.h"

class AIO {
public:
    explicit AIO(int fd) : fd(fd) {
        // fd will set noblock in ASock
        this->aSock = new ASock(fd);
    }

    int read(unsigned char *buffer, size_t size, int timeout);

    int write(unsigned char *buffer, size_t size, int timeout);

    ~AIO() {
        delete aSock;
    }

private:
    int fd;
    ASock *aSock;
};


#endif //COROUTINE_AIO_H
