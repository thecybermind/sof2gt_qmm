/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __SOF2GT_QMM_VERSION_H__
#define __SOF2GT_QMM_VERSION_H__

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define SOF2GT_QMM_VERSION_MAJOR	1
#define SOF2GT_QMM_VERSION_MINOR	0
#define SOF2GT_QMM_VERSION_REV	    0

#define SOF2GT_QMM_VERSION		STRINGIFY(SOF2GT_QMM_VERSION_MAJOR) "." STRINGIFY(SOF2GT_QMM_VERSION_MINOR) "." STRINGIFY(SOF2GT_QMM_VERSION_REV)

#if defined(_WIN32)
#define SOF2GT_QMM_OS             "Windows"
#ifdef _WIN64
#define SOF2GT_QMM_ARCH           "x86_64"
#else
#define SOF2GT_QMM_ARCH           "x86"
#endif
#elif defined(__linux__)
#define SOF2GT_QMM_OS             "Linux"
#ifdef __LP64__
#define SOF2GT_QMM_ARCH           "x86_64"
#else
#define SOF2GT_QMM_ARCH           "x86"
#endif
#endif

#define SOF2GT_QMM_VERSION_DWORD	SOF2GT_QMM_VERSION_MAJOR , SOF2GT_QMM_VERSION_MINOR , SOF2GT_QMM_VERSION_REV , 0
#define SOF2GT_QMM_COMPILE		    __TIME__ " " __DATE__
#define SOF2GT_QMM_BUILDER		    "Kevin Masterson"

#endif // __SOF2GT_QMM_VERSION_H__
