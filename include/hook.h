/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __SOF2GT_QMM_HOOK_H__
#define __SOF2GT_QMM_HOOK_H__

bool hook_enable(const char* gametype);
bool hook_disable();

#if defined(__linux__)

    #include <dlfcn.h>
    #include <unistd.h> 
    #include <limits.h>
    #include <ctype.h>
    #include <sys/stat.h>

#endif // __linux__

#endif // __SOF2GT_QMM_HOOK_H__
