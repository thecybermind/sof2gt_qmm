/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <stdint.h>

#if defined(_WIN32)
    #define DLLEXPORT __declspec(dllexport)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #define dlopen(file, x)		((void*)LoadLibrary(file))
    #define dlsym(dll, func)	((void*)GetProcAddress((HMODULE)(dll), (func)))
    #define dlclose(dll)		FreeLibrary((HMODULE)(dll))
#elif defined(__linux__)
    #define DLLEXPORT __attribute__((visibility("default")))
    #include <dlfcn.h>
#endif

typedef void (*dllEntry_t)(void*);
typedef intptr_t(*vmMain_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);

dllEntry_t g_dllEntry = NULL;
vmMain_t g_vmMain = NULL;

DLLEXPORT void dllEntry(void* syscall) {
    void* dll = dlopen("base/mp/qmmaddons/sof2gt/sof2gt_qmm_SOF2MP.dll", RTLD_NOW);
    if (!dll)
        return;
    g_dllEntry = (dllEntry_t)dlsym(dll, "dllEntry");
    if (!g_dllEntry) {
        dlclose(dll);
        return;
    }
    g_vmMain = (vmMain_t)dlsym(dll, "vmMain");
    if (!g_vmMain) {
        dlclose(dll);
        return;
    }
    g_dllEntry(syscall);
}


DLLEXPORT intptr_t vmMain(intptr_t cmd, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) {
    return g_vmMain(cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
}
