/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1

#include <qmmapi.h>

#include <map>
#include <string.h>
#include <cstdint>

#include "version.h"
#include "game.h"
#include "util.h"
#include "sof2gt_api.h"
#include "main.h"
#include "hook.h"
#include "qvm.h"

pluginres_t* g_result = nullptr;
plugininfo_t g_plugininfo = {
	QMM_PIFV_MAJOR,									// plugin interface version major
	QMM_PIFV_MINOR,									// plugin interface version minor
	"SoF2GT_QMM",									// name of plugin
	SOF2GT_QMM_VERSION,								// version of plugin
	"Hook SoF2MP's gametype qvms",					// description of plugin
	SOF2GT_QMM_BUILDER,								// author of plugin
	"https://github.com/thecybermind/sof2gt_qmm/",	// website of plugin
	"SOF2GT",										// log tag
};
eng_syscall_t g_syscall = nullptr;
mod_vmMain_t g_vmMain = nullptr;
pluginfuncs_t* g_pluginfuncs = nullptr;
pluginvars_t* g_pluginvars = nullptr;

// gametype module information
void* gt_dll = nullptr;
qvm_t gt_qvm;

// track if we shouldn't load (no plugins or hook failed)
bool g_disabled = false;

// track if we shutdown
bool g_shutdown = false;

// stuff to pass to plugins
static sof2gt_pluginvars_t gt_pluginvars = {
	"",			// gt_gametype
	PLID,		// gt_sof2gt_plid
	0,			// gt_return
	QMM_UNUSED,	// gt_result
	nullptr,	// gt_syscall
	nullptr,	// gt_vmMain
};

// store our plugins
static std::map<plid_t, sof2gt_plugin_t> s_plugins;

// attempt to load DLL gametype mod
static bool s_load_dll(const char* file);
// attempt to load QVM gametype mod
static bool s_load_qvm(const char* file);


C_DLLEXPORT void QMM_Query(plugininfo_t** pinfo) {
	// give QMM our plugin info struct
	QMM_GIVE_PINFO();
}


C_DLLEXPORT int QMM_Attach(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, pluginvars_t* pluginvars) {
	QMM_SAVE_VARS();

	// make sure this DLL is loaded only in the right engine
	if (strcmp(QMM_GETGAMEENGINE(), GAME_STR) != 0)
		return 0;

	return 1;
}


C_DLLEXPORT void QMM_Detach() {
}


C_DLLEXPORT intptr_t QMM_vmMain(intptr_t cmd, intptr_t* args) {
	if (g_disabled)
		QMM_RET_IGNORED(0);

	if (cmd == GAME_INIT) {
		QMM_WRITEQMMLOG("SoF2GT loaded!\n", QMMLOG_NOTICE);

		// pass gt plugin variables to plugins
		QMM_PLUGIN_BROADCAST("SOF2GT_Attach", &gt_pluginvars, sizeof(gt_pluginvars));

		// if we didn't attach to any plugins, then disable ourselves
		if (s_plugins.empty()) {
			g_disabled = true;
			QMM_WRITEQMMLOG("No SoF2GT plugins found, disabling!\n", QMMLOG_INFO);
		}
		else {
			QMM_WRITEQMMLOG(QMM_VARARGS("%d SoF2GT plugin(s) found!\n", s_plugins.size()), QMMLOG_INFO);
		}
	}
	else if (cmd == GAME_SHUTDOWN) {
		if (gt_dll)
			dlclose(gt_dll);
		qvm_unload(&gt_qvm);
	}

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_syscall(intptr_t cmd, intptr_t* args) {
	if (g_disabled)
		QMM_RET_IGNORED(0);

	// this cmd is what the mod calls to initialize a gametype qvm/dll
	if (cmd == G_GT_INIT) {
		// save the gametype string so we know what gametype file to load
		const char* gametype = (const char*)args[0];
		strncpyz(gt_pluginvars.gt_gametype, gametype, sizeof(gt_pluginvars.gt_gametype));
		
		// install LoadLibraryA/dlopen hook in server binary to intercept loading of gametype dll
		// if cannot install hook, then disable ourselves
		if (!hook_enable(gt_pluginvars.gt_gametype)) {
			g_disabled = true;
			QMM_WRITEQMMLOG("Failed to install GetProcAddress/dlopen hook, disabling!\n", QMMLOG_NOTICE);

			QMM_RET_IGNORED(0);
		}
		QMM_WRITEQMMLOG("Hook installed for gametype module!\n", QMMLOG_INFO);
	}

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_vmMain_Post(intptr_t cmd, intptr_t* args) {
	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_syscall_Post(intptr_t cmd, intptr_t* args) {
	if (g_disabled)
		QMM_RET_IGNORED(0);

	// engine is done loading gametype qvm/dll (actually us)
	if (cmd == G_GT_INIT) {
		// unload hook
		if (hook_disable())
			QMM_WRITEQMMLOG("Hook uninstalled for gametype module!\n", QMMLOG_INFO);
	}
	QMM_RET_IGNORED(0);
}


C_DLLEXPORT void QMM_PluginMessage(plid_t from_plid, const char* message, void* buf, intptr_t buflen, int is_broadcast) {
	if (g_disabled)
		return;

	constexpr int NUM_SOF2GT_PLUGIN_FUNCS = 4;

	// get sof2gt hook functions from other plugins
	if (!strcmp(message, "SOF2GT_GT_GiveFuncs")) {
		if (buflen != NUM_SOF2GT_PLUGIN_FUNCS * sizeof(sof2gt_pluginfunc_t)) {
			QMM_WRITEQMMLOG(QMM_VARARGS("Unexpected buflen in SOF2GT_GT_GiveFuncs handler: got %d, expected %d\n", buflen, NUM_SOF2GT_PLUGIN_FUNCS * sizeof(sof2gt_pluginfunc_t)), QMMLOG_DEBUG);
			return;
		}
		sof2gt_pluginfunc_t* funcs = (sof2gt_pluginfunc_t*)buf;
		sof2gt_plugin_t plugin = {
			from_plid,
			funcs[0],
			funcs[1],
			funcs[2],
			funcs[3],
		};
		s_plugins[from_plid] = plugin;
	}
	(void)is_broadcast;
}


// entry point: handle gametype vmMain calls from engine
C_DLLEXPORT intptr_t vmMain(intptr_t cmd, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) {
	if (cmd == GAMETYPE_INIT) {
		QMM_WRITEQMMLOG(QMM_VARARGS("Gametype '%s' initialized!\n", gt_pluginvars.gt_gametype), QMMLOG_NOTICE);
	}

	intptr_t args[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6 };

	// store max plugin result
	pluginres_t max_result = QMM_UNUSED;
	// return value from plugin call
	intptr_t plugin_ret = 0;
	// return value from mod call
	intptr_t mod_ret = 0;
	// return value to pass back to the engine (either mod_ret, or a plugin_ret from QMM_OVERRIDE/QMM_SUPERCEDE result)
	intptr_t final_ret = 0;

	// begin passing calls to plugins' SOF2GT_GT_vmMain functions
	for (auto& p : s_plugins) {
		// skip if plugin doesn't have this function
		if (!p.second.SOF2GT_GT_vmMain)
			continue;

		gt_pluginvars.gt_result = QMM_UNUSED;

		// call plugin's GT_vmMain and store return value
		plugin_ret = p.second.SOF2GT_GT_vmMain(cmd, args);

		// set new max result
		if (gt_pluginvars.gt_result > max_result)
			max_result = gt_pluginvars.gt_result;

		// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
		if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
			final_ret = plugin_ret;
	}

	// call real vmMain function (unless a plugin resulted in QMM_SUPERCEDE)
	if (gt_pluginvars.gt_result < QMM_SUPERCEDE)
		mod_ret = gt_pluginvars.gt_vmMain(cmd, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);

	// if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual mod's return value back to the engine
	if (gt_pluginvars.gt_result < QMM_OVERRIDE)
		final_ret = mod_ret;

	// begin passing calls to plugins' SOF2GT_GT_vmMain_Post functions
	for (auto& p : s_plugins) {
		// skip if plugin doesn't have this function
		if (!p.second.SOF2GT_GT_vmMain_Post)
			continue;

		gt_pluginvars.gt_result = QMM_UNUSED;

		// call plugin's GT_vmMain_Post and store return value
		plugin_ret = p.second.SOF2GT_GT_vmMain_Post(cmd, args);

		// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
		if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
			final_ret = plugin_ret;
	}

	// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
	if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
		final_ret = gt_pluginvars.gt_return;

	return final_ret;
}


// handle gametype syscalls from gametype mod (DLL or QVM) 
intptr_t SOF2GT_syscall(intptr_t cmd, ...) {
	// pull args from ...
	intptr_t args[SOF2GT_SYSCALL_ARGS] = { };
	va_list arglist;
	va_start(arglist, cmd);
	for (int i = 0; i < SOF2GT_SYSCALL_ARGS; ++i)
		args[i] = va_arg(arglist, intptr_t);
	va_end(arglist);

	// store max plugin result
	pluginres_t max_result = QMM_UNUSED;
	// return value from plugin call
	intptr_t plugin_ret = 0;
	// return value from engine call
	intptr_t eng_ret = 0;
	// return value to pass back to the mod (either eng_ret, or a plugin_ret from QMM_OVERRIDE/QMM_SUPERCEDE result)
	intptr_t final_ret = 0;

	// begin passing calls to plugins' SOF2GT_GT_syscall functions
	for (auto& p : s_plugins) {
		// skip if plugin doesn't have this function
		if (!p.second.SOF2GT_GT_syscall)
			continue;

		gt_pluginvars.gt_result = QMM_UNUSED;

		// call plugin's GT_syscall and store return value
		plugin_ret = p.second.SOF2GT_GT_syscall(cmd, args);

		// set new max result
		if (gt_pluginvars.gt_result > max_result)
			max_result = gt_pluginvars.gt_result;

		// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
		if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
			final_ret = plugin_ret;
	}

	// call real syscall function (unless a plugin resulted in QMM_SUPERCEDE)
	if (gt_pluginvars.gt_result < QMM_SUPERCEDE)
		eng_ret = gt_pluginvars.gt_syscall(cmd, args[0], args[1], args[2], args[3], args[4], args[5]);

	// if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual mod's return value back to the engine
	if (gt_pluginvars.gt_result < QMM_OVERRIDE)
		final_ret = eng_ret;

	// begin passing calls to plugins' SOF2GT_GT_syscall_Post functions
	for (auto& p : s_plugins) {
		// skip if plugin doesn't have this function
		if (!p.second.SOF2GT_GT_syscall_Post)
			continue;

		gt_pluginvars.gt_result = QMM_UNUSED;

		// call plugin's GT_syscall_Post and store return value
		plugin_ret = p.second.SOF2GT_GT_syscall_Post(cmd, args);

		// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
		if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
			final_ret = plugin_ret;
	}

	// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
	if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
		final_ret = gt_pluginvars.gt_return;

	return final_ret;
}


// pass gametype vmMain calls into QVM gametype mod
// this is given to SOFT2GT plugins
intptr_t SOFT2GT_qvm_vmmain(intptr_t cmd, ...) {
	// if qvm isn't loaded, we need to error
	if (!gt_qvm.memory) {
		if (!g_shutdown) {
			g_shutdown = true;
			QMM_WRITEQMMLOG(QMM_VARARGS("SOFT2GT_qvm_vmmain(%d): QVM unloaded during previous execution due to a run-time error\n", cmd), QMMLOG_FATAL);
			g_syscall(G_ERROR, "\n\n=========\nFatal SOFT2GT Error:\nThe QVM was unloaded during previous execution due to a run-time error.\n=========\n");
		}
		return 0;
	}

	// pull args from ... as ints, and also include cmd at the front
	intptr_t args[SOF2GT_VMMAIN_ARGS + 1] = { cmd };
	va_list arglist;
	va_start(arglist, cmd);
	for (int i = 0; i < SOF2GT_VMMAIN_ARGS; ++i)
		args[i + 1] = (int) va_arg(arglist, intptr_t);
	va_end(arglist);

	// pass array and size to qvm
	return qvm_exec(&gt_qvm, SOF2GT_VMMAIN_ARGS + 1, args);
}


// handle gametype syscalls from QVM gametype mod (continues to SOF2GT_syscall)
int SOF2GT_qvm_syscall(uint8_t* membase, int cmd, int* args) {
	intptr_t ret = 0;

	switch (cmd) {
	case GT_MILLISECONDS:					// (void)
		ret = SOF2GT_syscall(cmd);
		break;
	case GT_SIN:							// (double x)
	case GT_COS:							// (double x)
	case GT_SQRT:							// (double f)
	case GT_FLOOR:							// (double f)
	case GT_CEIL:							// (double f)
	case GT_ACOS:							// (double x)
	case GT_ASIN:							// not used in SDK, but probably (double x)
	case GT_RESETITEM:						// void (int itemid)
	case GT_STARTGLOBALSOUND:				// void (int soundid)
	case GT_RESTART:						// void (int delay)
		ret = SOF2GT_syscall(cmd, VMARG(0));
		break;
	case GT_PRINT:							// (const char *string)
	case GT_ERROR:							// (const char *string)
	case GT_CVAR_UPDATE:					// (mCvar_t *vmCvar)
	case GT_CVAR_VARIABLE_INTEGER_VALUE:	// (const char *var_name)
	case GT_REGISTERSOUND:					// int  (const char* filename)
	case GT_REGISTEREFFECT:					// int	(const char* name)
	case GT_REGISTERICON:					// int	(const char* icon)
	case GT_USETARGETS:						// void (const char* targetname)
		ret = SOF2GT_syscall(cmd, VMPTR(0));
		break;
	case GT_ATAN2:							// (double y, double x)
	case GT_DOESCLIENTHAVEITEM:				// bool (int clientid, int itemid)
	case GT_ADDTEAMSCORE:					// void (team_t team, int score)
	case GT_ADDCLIENTSCORE:					// void (int clientid, int score)
	case GT_GIVECLIENTITEM:					// void (int clientid, int itemid)
	case GT_TAKECLIENTITEM:					// void (int clientid, int itemid)
	case GT_SETHUDICON:						// void	(int index, int icon)
		ret = SOF2GT_syscall(cmd, VMARG(0), VMARG(1));
		break;
	case GT_TESTPRINTINT:					// (char* msg, int i)
	case GT_TESTPRINTFLOAT:					// (char* msg, float f)
		ret = SOF2GT_syscall(cmd, VMPTR(0), VMARG(1));
		break;
	case GT_TEXTMESSAGE:					// void (int clientid, const char* message)
	case GT_RADIOMESSAGE:					// void (int clientid, const char* message)
	case GT_GETCLIENTORIGIN:				// void (int clientid, vec3_t origin)
	case GT_STARTSOUND:						// void (int soundid, vec3_t origin)
		ret = SOF2GT_syscall(cmd, VMARG(0), VMPTR(1));
		break;
	case GT_CVAR_SET:						// (const char *var_name, const char *value)
	case GT_PERPENDICULARVECTOR:			// (vec3_t dst, const vec3_t src)
		ret = SOF2GT_syscall(cmd, VMPTR(0), VMPTR(1));
		break;
	case GT_MEMSET:							// (void* dest, int c, size_t count)
		ret = SOF2GT_syscall(cmd, VMPTR(0), VMARG(2), VMARG(2));
		break;
	case GT_GETCLIENTNAME:					// void (int clientid, const char* buffer, int buffersize)
	case GT_GETCLIENTITEMS:					// void (int clientid, int* buffer, int buffersize)
	case GT_GETTRIGGERTARGET:				// void (int triggerid, char* buffer, int buffersize)
	case GT_GETCLIENTLIST:					// int  (team_t team, int* clients, int clientcount)
		ret = SOF2GT_syscall(cmd, VMARG(0), VMPTR(1), VMARG(2));
		break;
	case GT_CVAR_VARIABLE_STRING_BUFFER:	// (const char *var_name, char *buffer, int bufsize)
	case GT_MEMCPY:							// (void* dest, const void* src, size_t count)
	case GT_STRNCPY:						// (char* strDest, const char* strSource, size_t count)
		ret = SOF2GT_syscall(cmd, VMPTR(0), VMPTR(1), VMARG(2));
		break;
	case GT_REGISTERITEM:					// bool (int itemid, const char* name, gtItemDef_t* def)
	case GT_REGISTERTRIGGER:				// bool (int trigid, const char* name, gtTriggerDef_t* def)
	case GT_PLAYEFFECT:						// void	(int effect, vec3_t origin, vec3_t angles)
	case GT_SPAWNITEM:						// void (int itemid, vec3_t origin, vec3_t angles)
		ret = SOF2GT_syscall(cmd, VMARG(0), VMPTR(1), VMPTR(2));
		break;
	case GT_MATRIXMULTIPLY:					// (float in1[3][3], float in2[3][3], float out[3][3])
		ret = SOF2GT_syscall(cmd, VMPTR(0), VMPTR(1), VMPTR(2));
		break;
	case GT_CVAR_REGISTER:					// (vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags)
		ret = SOF2GT_syscall(cmd, VMPTR(0), VMPTR(1), VMPTR(2), VMARG(3));
		break;
	case GT_ANGLEVECTORS:					// (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
		ret = SOF2GT_syscall(cmd, VMPTR(0), VMPTR(1), VMPTR(2), VMPTR(3));
		break;
	default:
		ret = 0;
	}

	return ret;
}


// entry point: handle gametype load from engine
C_DLLEXPORT void dllEntry(eng_syscall_t syscall) {
	// store gametype syscall from engine
	gt_pluginvars.gt_syscall = syscall;

	QMM_WRITEQMMLOG(QMM_VARARGS("Gametype hook DLL loaded for gametype '%s'\n", gt_pluginvars.gt_gametype), QMMLOG_NOTICE);

	// load gametype mod file
	const char* modpath = QMM_VARARGS("base/mp/qmm_gt_%sx86.dll", gt_pluginvars.gt_gametype);
	if (!s_load_dll(modpath)) {
		modpath = QMM_VARARGS("vm/gt_%s.qvm", gt_pluginvars.gt_gametype);
		if (!s_load_qvm(modpath)) {
			// unfortunately at this point, we can't back out safely, so G_ERROR it is
			g_shutdown = true;
			g_syscall(G_ERROR, QMM_VARARGS("Could not load DLL or QVM for gametype '%s'\n", gt_pluginvars.gt_gametype));
			return;
		}
	}

	QMM_WRITEQMMLOG(QMM_VARARGS("Successfully loaded %s for gametype '%s'\n", (gt_dll ? "DLL" : "QVM"), gt_pluginvars.gt_gametype), QMMLOG_NOTICE);
}


// attempt to load DLL gametype mod
static bool s_load_dll(const char* file) {
	mod_dllEntry_t gt_dllEntry;

	gt_dll = dlopen(file, RTLD_NOW);
	if (!gt_dll) {
		QMM_WRITEQMMLOG(QMM_VARARGS("s_load_dll(\"%s\"): Could not open DLL file for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		goto fail;
	}
	gt_dllEntry = (mod_dllEntry_t)dlsym(gt_dll, "dllEntry");
	if (!gt_dllEntry) {
		QMM_WRITEQMMLOG(QMM_VARARGS("s_load_dll(\"%s\"): Could not find 'dllEntry' in DLL for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		goto fail;
	}
	gt_pluginvars.gt_vmMain = (mod_vmMain_t)dlsym(gt_dll, "vmMain");
	if (!gt_pluginvars.gt_vmMain) {
		QMM_WRITEQMMLOG(QMM_VARARGS("s_load_dll(\"%s\"): Could not find 'vmMain' in DLL for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		goto fail;
	}

	// pass our syscall to gametype DLL
	gt_dllEntry(SOF2GT_syscall);

	return true;

fail:
	if (gt_dll)
		dlclose(gt_dll);
	gt_dll = nullptr;
	return false;
}


// attempt to load QVM gametype mod
static bool s_load_qvm(const char* file) {
	int fpk3;
	intptr_t filelen;
	uint8_t* filemem = nullptr;
	int loaded;

	// load file using engine functions to read into pk3s if necessary
	filelen = g_syscall(G_FS_FOPEN_FILE, file, &fpk3, FS_READ);
	if (filelen <= 0) {
		QMM_WRITEQMMLOG(QMM_VARARGS("s_load_qvm(\"%s\"): Could not open QVM for reading for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		g_syscall(G_FS_FCLOSE_FILE, fpk3);
		goto fail;
	}
	filemem = (uint8_t*)malloc(filelen);

	g_syscall(G_FS_READ, filemem, filelen, fpk3);
	g_syscall(G_FS_FCLOSE_FILE, fpk3);

	// attempt to load mod
	loaded = qvm_load(&gt_qvm, filemem, filelen, SOF2GT_qvm_syscall, true, nullptr);	// true = verify_data
	if (!loaded) {
		QMM_WRITEQMMLOG(QMM_VARARGS("s_load_qvm(\"%s\"): QVM load failed for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		goto fail;
	}

	// special function to call into QVM
	gt_pluginvars.gt_vmMain = SOFT2GT_qvm_vmmain;

	return true;

fail:
	if (filemem)
		free(filemem);
	filemem = nullptr;
	return false;
}
