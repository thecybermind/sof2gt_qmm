/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef SOF2GT_QMM_MAIN_H
#define SOF2GT_QMM_MAIN_H

#include <cstdint>
#include "sof2gt_api.h"
#include "qvm.h"

#define SOF2GT_SYSCALL_ARGS 6
#define SOF2GT_VMMAIN_ARGS  7

// gametype module information

// store dll handle for gametype mod
extern void* gt_dll;
// qvm virtual machine
extern qvm_t gt_qvm;

// track if we shouldn't load (no plugins or hook failed)
extern bool g_disabled;

// track if we shutdown
extern bool g_shutdown;

// plugin stuff
typedef intptr_t (*sof2gt_pluginfunc_t)(intptr_t cmd, intptr_t* args);
typedef struct sof2gt_plugin_s {
    plid_t plid;
    sof2gt_pluginfunc_t SOF2GT_GT_vmMain;
    sof2gt_pluginfunc_t SOF2GT_GT_vmMain_Post;
    sof2gt_pluginfunc_t SOF2GT_GT_syscall;
    sof2gt_pluginfunc_t SOF2GT_GT_syscall_Post;
} sof2gt_plugin_t;

// handle syscall from gametype mod (DLL or QVM)
intptr_t SOF2GT_syscall(intptr_t cmd, ...);

// pass vmMain calls into QVM gametype mod
intptr_t SOFT2GT_qvm_vmmain(intptr_t cmd, ...);

// handle syscalls from QVM gametype mod (redirects to SOF2GT_syscall)
int SOF2GT_qvm_syscall(uint8_t* membase, int cmd, int* args);
    
// this gets an argument value (evaluate to an intptr_t)
#define vmarg(arg)	(intptr_t)args[arg]
// this adds the base VM address pointer to an argument value (evaluate to a pointer)
#define vmptr(arg)	(args[arg] ? membase + args[arg] : nullptr)
// this subtracts the base VM address pointer from a value, for returning from syscall (this should evaluate to an int)
#define vmret(ptr)	(int)(ptr ? (intptr_t)ptr - (intptr_t)membase : 0)

#endif // SOF2GT_QMM_MAIN_H
