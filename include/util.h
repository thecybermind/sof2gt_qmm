/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __SOF2GT_QMM_UTIL_H__
#define __SOF2GT_QMM_UTIL_H__

#define COUNTOF(arr)  (sizeof(arr) / sizeof(arr[0]))

// "safe" strncpy that always null-terminates
char* strncpyz(char* dest, const char* src, size_t count);

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>

#define PATH_MAX			4096
#define dlopen(file, x)		((void*)LoadLibrary(file))
#define dlsym(dll, func)	((void*)GetProcAddress((HMODULE)(dll), (func)))
#define dlclose(dll)		FreeLibrary((HMODULE)(dll))

#elif defined(__linux__)

#include <dlfcn.h>
#include <unistd.h> 
#include <limits.h>
#include <ctype.h>
#include <sys/stat.h>

#endif // __linux__

#endif // __SOF2GT_QMM_UTIL_H__
