/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __SOF2GT_QMM_SOF2GT_PLUGIN_H__
#define __SOF2GT_QMM_SOF2GT_PLUGIN_H__

#include <cstdint>
#include <qmmapi.h>

struct sof2gt_plugininfo_t {
	char gt_gametype[32];
	intptr_t gt_return;
	pluginres_t gt_result;
	eng_syscall_t gt_syscall;
	mod_vmMain_t gt_vmMain;
};

#endif // __SOF2GT_QMM_SOF2GT_PLUGIN_H__
