/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __SOF2GT_QMM_MAIN_H__
#define __SOF2GT_QMM_MAIN_H__

#include <cstdint>
#include "sof2gt_plugin.h"
#include "qvm.h"

#define SOF2GT_SYSCALL_ARGS 6
#define SOF2GT_VMMAIN_ARGS  7

// gametype module information

// store dll handle for gametype mod
extern void* gt_dll;
// qvm virtual machine
extern qvm_t gt_qvm;
// stuff to pass to plugins
extern sof2gt_plugininfo_t gt_pluginvars;

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

#endif // __SOF2GT_QMM_MAIN_H__
