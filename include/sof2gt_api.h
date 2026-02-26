/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

// public API for SOF2GT_QMM plugins

#ifndef SOF2GT_QMM_SOF2GT_API_H
#define SOF2GT_QMM_SOF2GT_API_H

#include <cstdint>
#include <qmmapi.h>

typedef struct {
	char gt_gametype[32];
    plugin_id gt_sof2gt_plid;
	intptr_t gt_return;
	plugin_res gt_result;
	eng_syscall gt_syscall;
	mod_vmMain gt_vmMain;
} sof2gt_plugin_vars;

extern sof2gt_plugin_vars* sof2gt_pluginvars;

#define SOF2GT_SAVE_VARS() sof2gt_pluginvars = (sof2gt_plugin_vars*)buf

extern intptr_t SOF2GT_GT_vmMain(intptr_t cmd, intptr_t* args);
extern intptr_t SOF2GT_GT_vmMain_Post(intptr_t cmd, intptr_t* args);
extern intptr_t SOF2GT_GT_syscall(intptr_t cmd, intptr_t* args);
extern intptr_t SOF2GT_GT_syscall_Post(intptr_t cmd, intptr_t* args);

#define SOF2GT_GIVE_FUNCS() do { \
                                intptr_t(*funcs[])(intptr_t cmd, intptr_t* args) = { \
                                    &SOF2GT_GT_vmMain, \
                                    &SOF2GT_GT_vmMain_Post, \
                                    &SOF2GT_GT_syscall, \
                                    &SOF2GT_GT_syscall_Post, \
                                }; \
                                QMM_PLUGIN_SEND(sof2gt_pluginvars->gt_sof2gt_plid, "SOF2GT_GT_GiveFuncs", &funcs, sizeof(funcs)); \
                            } while(0)

// macros to help set the plugin result value
#define SOF2GT_SET_RESULT(res)    sof2gt_pluginvars->gt_result = (pluginres_t)(res)
#define SOF2GT_RETURN(res, ret)   return (SOF2GT_SET_RESULT(res), (ret))
#define SOF2GT_RET_ERROR(ret)     SOF2GT_RETURN(QMM_ERROR, (ret))
#define SOF2GT_RET_IGNORED(ret)	  SOF2GT_RETURN(QMM_IGNORED, (ret))
#define SOF2GT_RET_OVERRIDE(ret)  SOF2GT_RETURN(QMM_OVERRIDE, (ret))
#define SOF2GT_RET_SUPERCEDE(ret) SOF2GT_RETURN(QMM_SUPERCEDE, (ret))


#endif // SOF2GT_QMM_SOF2GT_API_H
