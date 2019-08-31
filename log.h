//
// Created by explorer on 8/25/19.
//

#ifndef COROUTINE_LOG_H
#define COROUTINE_LOG_H
#include <stdio.h>

int logger(const char *level, const char *pFile, const char *pFuncName, int iLineNumb, FILE *pLogHandle, const char *fmt, ...);

#define INFO "info", __FILE__ , __FUNCTION__, __LINE__
#define WARN "warning", __FILE__ , __FUNCTION__, __LINE__
#define ERR "error", __FILE__ , __FUNCTION__, __LINE__
#define DEBUG "debug", __FILE__ , __FUNCTION__, __LINE__
#endif //COROUTINE_LOG_H
