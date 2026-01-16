/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1
#include "version.h"
#include <qmmapi.h>
#include <cstring>
#include <string>
#include "game.h"
#include "util.h"


// "safe" strncpy that always null-terminates
char* strncpyz(char* dest, const char* src, size_t count) {
    char* ret = strncpy(dest, src, count);
    dest[count - 1] = '\0';
    return ret;
}


// allow qvm.c to log without needing to include QMM/game headers
extern "C" void log_c(int severity, const char* tag, const char* fmt, ...) {
    (void)tag;

    va_list	argptr;
    static char buf[1024];

    va_start(argptr, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argptr);
    va_end(argptr);

    QMM_WRITEQMMLOG(PLID, buf, severity);
}
