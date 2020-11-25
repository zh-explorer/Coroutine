//
// Created by explorer on 7/16/19.
//

#include "array_buf.h"
#include <cstring>
#include <cstdlib>
#include "../log.h"

__buffer::__buffer() {
    clean();
}

unsigned int __buffer::length() {
    return static_cast<unsigned int>(end - start);
}

unsigned int __buffer::remain() {
    return static_cast<unsigned int>((buf + BLOCK_SIZE) - end);
}

void __buffer::read(unsigned char *buffer, unsigned int size) {
    if (size > length()) {
        logger(ERR, stderr, "internal error");
        abort();
    }
    memcpy(buffer, start, size);
    start += size;
}

void __buffer::write(unsigned char *buffer, unsigned int size) {
    if (size > remain()) {
        logger(ERR, stderr, "internal error");
        abort();
    }
    memcpy(end, buffer, size);
    end += size;
}

// clean the buffer for reuse
void __buffer::clean() {
    memset(buf, 0, BLOCK_SIZE);
    start = buf;
    end = buf;
    next = nullptr;
}

void array_buf::init() {
    head = new __buffer();
    tail = head;
    cache = nullptr;
    len = 0;
}

array_buf::array_buf() {
    init();
}

array_buf::array_buf(unsigned char *buffer, unsigned int size) {
    init();
    write(buffer, size);
}

unsigned int array_buf::read(unsigned char *buffer, unsigned int size) {
    unsigned int already_read = 0;
    unsigned int read_result;
    while (already_read < size) { // already_read != size
        read_result = this->read_block(buffer + already_read, size - already_read);
        if (read_result == 0) {   // no more data
            return already_read;
        }
        already_read += read_result;
    }
    return size;    // all data request is read
}

// read data from first block and return size of data read
unsigned int array_buf::read_block(unsigned char *buffer, unsigned int size) {
    unsigned data_size = this->head->length();
    if (!data_size) {
        return 0;
    } else if (data_size > size) {
        this->head->read(buffer, size);
        return size;
    } else {
        this->head->read(buffer, data_size);
        this->pop_block();  // we read all data from block, pop it
        return data_size;
    }
}

//unsigned int array_buf::read(unsigned char *buffer, unsigned int size) {
//    unsigned int already_read = 0;
//    unsigned int result;
//    result = this->head->length();
//    if (result >= size) {
//        this->head->read(buffer, size);
//
//        if (result == size) {
//            pop_block();
//        }
//        this->len -= size;
//        return size;
//    } else if (size <= length()) {
//
//        while (true) {
//            result = this->head->length();
//            if (result >= (size - already_read)) {
//                head->read(buffer + already_read, size - already_read);
//                if (result == (size - already_read)) {
//                    pop_block();
//                }
//                this->len -= size;
//                return size;
//            } else {
//                this->head->read(buffer + already_read, result);
//                pop_block();
//                already_read += result;
//            }
//        }
//    } else { // size > length()
//        while (this->head->length()) {
//            result = this->head->length();
//            this->head->read(buffer + already_read, result);
//            already_read += result;
//            this->pop_block();
//        }
//        result = this->len;
//        this->len = 0;
//        return result;
//    }
//}
//
//unsigned int array_buf::write2(unsigned char *buffer, unsigned int size) {
//    unsigned int re;
//    unsigned int already_write = 0;
//    re = tail->remain();
//    if (size <= re) {
//        tail->write(buffer, size);
//        if (size == re) {
//            push_block();
//        }
//        len += size;
//        return size;
//    } else {
//        while (true) {
//            re = tail->remain();
//            if (re > (size - already_write)) {
//                tail->write(buffer + already_write, size - already_write);
//                len += size;
//                return size;
//            }
//            tail->write(buffer + already_write, re);
//            already_write += re;
//            push_block();
//        }
//    }
//}

unsigned int array_buf::write(unsigned char *buffer, unsigned int size) {
    unsigned int already_write = 0;
    unsigned int write_result;
    while (already_write < size) {
        write_result = this->write_block(buffer + already_write, size - already_write);
        already_write += write_result;
    }
    return size;
}


unsigned int array_buf::write_block(unsigned char *buffer, unsigned int size) {
    unsigned space_remain = this->tail->remain();
    if (!space_remain) {
        this->push_block();
        space_remain = this->tail->remain();
    }
    if (space_remain > size) {
        this->tail->write(buffer, size);
        return size;
    } else {
        this->tail->write(buffer, space_remain);
        return space_remain;
    }
}


// pop block from head
void array_buf::pop_block() {
    __buffer *p;
    if (this->head->next) {
        p = this->head;
        this->head = this->head->next;
        if (this->cache) {
            delete p;
        } else {
            this->cache = p;
            p->clean();
        }
    } else {
        // we should remain at last one block in list, os that tail will never change in pop
        this->head->clean();
    }
}

// push a new block to tail
void array_buf::push_block() {
    __buffer *p;
    if (cache) {
        p = this->cache;      // cache is cleaned when set
        this->cache = nullptr;
    } else {
        p = new __buffer;
    }
    this->tail->next = p;     // there is at list one block in list, so head will never change in push
    this->tail = p;
}

unsigned int array_buf::length() const {
    return this->len;
}

array_buf::~array_buf() {
    delete this->cache;
    __buffer *p2;
    __buffer *p = this->head;
    while (p) {
        p2 = p;
        p = p->next;
        delete p2;
    }

}




