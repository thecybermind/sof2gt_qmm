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

#include <vector>
#include <string.h>

#include "version.h"
#include "game.h"
#include "util.h"
#include "sof2gt_plugin.h"
#include "main.h"
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

// stuff to pass to plugins
sof2gt_plugininfo_t gt_pluginvars = {
	"",			// gt_gametype
	0,			// gt_return
	QMM_UNUSED,	// gt_result
	nullptr,	// gt_syscall
	nullptr,	// gt_vmMain
};

// store the game's entity and client info
gentity_t* g_gents = nullptr;
intptr_t g_numgents = 0;
intptr_t g_gentsize = 0;
gclient_t* g_clients = nullptr;
intptr_t g_clientsize = 0;

// track if we shutdown
bool g_shutdown = 0;


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
	if (strcmp(QMM_GETGAMEENGINE(PLID), GAME_STR) != 0)
		return 0;

	return 1;
}


C_DLLEXPORT void QMM_Detach() {
}


C_DLLEXPORT intptr_t QMM_vmMain(intptr_t cmd, intptr_t* args) {
	if (cmd == GAME_INIT) {
		QMM_WRITEQMMLOG(PLID, "SoF2GT loaded!\n", QMMLOG_INFO);

		// pass result variable and return variable to plugins
		QMM_PLUGIN_BROADCAST(PLID, "SOF2GT_Attach", &gt_pluginvars, sizeof(gt_pluginvars));
	}
	else if (cmd == GAME_SHUTDOWN) {
		if (gt_dll)
			dlclose(gt_dll);
	}

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_syscall(intptr_t cmd, intptr_t* args) {
	if (cmd == G_LOCATE_GAME_DATA) {
		g_gents = (gentity_t*)(args[0]);
		g_numgents = args[1];
		g_gentsize = args[2];
		g_clients = (gclient_t*)(args[3]);
		g_clientsize = args[4];
	}

	// save the gametype so we know what gametype file to load
	if (cmd == G_GT_INIT) {
		const char* gametype = (const char*)args[0];
		strncpyz(gt_pluginvars.gt_gametype, gametype, sizeof(gt_pluginvars.gt_gametype));

		QMM_RET_IGNORED(0);
	}

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_vmMain_Post(intptr_t cmd, intptr_t* args) {

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_syscall_Post(intptr_t cmd, intptr_t* args) {

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT void QMM_PluginMessage(plid_t from_plid, const char* message, void* buf, intptr_t buflen) {
}


// handle syscall from gametype mod (DLL or QVM)
intptr_t SOF2GT_syscall(intptr_t cmd, ...) {
	// pull args from ..., put cmd in front
	intptr_t args[SOF2GT_SYSCALL_ARGS+1] = { cmd };
	va_list arglist;
	va_start(arglist, cmd);
	for (int i = 0; i < SOF2GT_SYSCALL_ARGS; ++i)
		args[i+1] = va_arg(arglist, intptr_t);
	va_end(arglist);

	// return value from mod call
	intptr_t mod_ret = 0;
	// return value to pass back to the engine (either mod_ret, or a plugin_ret from QMM_OVERRIDE/QMM_SUPERCEDE result)
	intptr_t final_ret = 0;

	// route to plugins
	gt_pluginvars.gt_return = 0;
	gt_pluginvars.gt_result = QMM_UNUSED;
	QMM_PLUGIN_BROADCAST(PLID, "SOF2GT_syscall", args, COUNTOF(args));

	// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
	if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
		final_ret = gt_pluginvars.gt_return;

	// call real syscall function (unless a plugin resulted in QMM_SUPERCEDE)
	if (gt_pluginvars.gt_result < QMM_SUPERCEDE)
		mod_ret = gt_pluginvars.gt_syscall(cmd, args[1], args[2], args[3], args[4], args[5], args[6]);

	// if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual mod's return value back to the engine
	if (gt_pluginvars.gt_result < QMM_OVERRIDE)
		final_ret = mod_ret;

	// route to plugins Post
	gt_pluginvars.gt_return = 0;
	gt_pluginvars.gt_result = QMM_UNUSED;
	QMM_PLUGIN_BROADCAST(PLID, "SOF2GT_syscall_Post", args, COUNTOF(args));

	// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
	if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
		final_ret = gt_pluginvars.gt_return;

	return final_ret;
}


// entry point: handle gametype vmMain calls from engine
C_DLLEXPORT intptr_t vmMain(intptr_t cmd, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) {
	if (g_shutdown)
		return 0;

	if (cmd == GAMETYPE_INIT) {
		QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "Gametype '%s' initialized!", gt_pluginvars.gt_gametype), QMMLOG_NOTICE);
	}

	intptr_t args[] = { cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6 };

	// return value from mod call
	intptr_t mod_ret = 0;
	// return value to pass back to the engine (either mod_ret, or a plugin_ret from QMM_OVERRIDE/QMM_SUPERCEDE result)
	intptr_t final_ret = 0;

	// route to plugins
	gt_pluginvars.gt_return = 0;
	gt_pluginvars.gt_result = QMM_UNUSED;
	QMM_PLUGIN_BROADCAST(PLID, "SOF2GT_vmMain", args, COUNTOF(args));

	// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
	if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
		final_ret = gt_pluginvars.gt_return;

	// call real vmMain function (unless a plugin resulted in QMM_SUPERCEDE)
	if (gt_pluginvars.gt_result < QMM_SUPERCEDE)
		mod_ret = gt_pluginvars.gt_vmMain(cmd, args[1], args[2], args[3], args[4], args[5], args[6], args[7]);

	// if no plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, return the actual mod's return value back to the engine
	if (gt_pluginvars.gt_result < QMM_OVERRIDE)
		final_ret = mod_ret;

	// route to plugins Post
	gt_pluginvars.gt_return = 0;
	gt_pluginvars.gt_result = QMM_UNUSED;
	QMM_PLUGIN_BROADCAST(PLID, "SOF2GT_vmMain_Post", args, COUNTOF(args));

	// if plugin resulted in QMM_OVERRIDE or QMM_SUPERCEDE, set final_ret to this return value
	if (gt_pluginvars.gt_result >= QMM_OVERRIDE)
		final_ret = gt_pluginvars.gt_return;

	return final_ret;
}


// pass vmMain calls into QVM gametype mod
// this is given to plugins
intptr_t SOFT2GT_qvm_vmmain(intptr_t cmd, ...) {
	// if qvm isn't loaded, we need to error
	if (!gt_qvm.memory) {
		if (!g_shutdown) {
			g_shutdown = true;
			QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "qvm_vmmain(%d): QVM unloaded during previous execution due to a run-time error\n", cmd), QMMLOG_FATAL);
			g_syscall(G_ERROR, "\n\n=========\nFatal QMM Error:\nThe QVM was unloaded during previous execution due to a run-time error.\n=========\n");
		}
		return 0;
	}

	// pull args from ...
	intptr_t args[SOF2GT_VMMAIN_ARGS] = {};
	va_list arglist;
	va_start(arglist, cmd);
	for (int i = 0; i < SOF2GT_VMMAIN_ARGS; ++i)
		args[i] = va_arg(arglist, intptr_t);
	va_end(arglist);

	// generate new int array from the intptr_t args, and also include cmd at the front
	int qvmargs[SOF2GT_VMMAIN_ARGS + 1] = { (int)cmd };
	for (int i = 0; i < SOF2GT_VMMAIN_ARGS; i++) {
		qvmargs[i + 1] = (int)args[i];
	}

	// pass array and size to qvm
	return qvm_exec(&gt_qvm, SOF2GT_VMMAIN_ARGS + 1, qvmargs);
}


// handle syscalls from QVM gametype mod (redirects to SOF2GT_syscall)
int SOF2GT_qvm_syscall(uint8_t* membase, int cmd, int* args) {
	intptr_t ret = 0;

	switch (cmd) {
	case GT_MILLISECONDS:					// ( void );
		ret = SOF2GT_syscall(cmd);
		break;
	case GT_SIN:							// (double)
	case GT_COS:							// (double)
	case GT_SQRT:							// (double)
	case GT_FLOOR:							// (double)
	case GT_CEIL:							// (double)
	case GT_ACOS:							// (double x)
	case GT_ASIN:							// not used, but probably (double x)
	case GT_RESETITEM:						// void ( int itemid );
	case GT_STARTGLOBALSOUND:				// void ( int soundid );
	case GT_RESTART:						// void ( int delay );
		ret = SOF2GT_syscall(cmd, args[0]);
		break;
	case GT_PRINT:							// ( const char *string );
	case GT_ERROR:							// ( const char *string );
	case GT_CVAR_UPDATE:					// ( vmCvar_t *vmCvar );
	case GT_CVAR_VARIABLE_INTEGER_VALUE:	// ( const char *var_name );
	case GT_REGISTERSOUND:					// int  ( const char* filename );
	case GT_REGISTEREFFECT:					// int	( const char* name );
	case GT_REGISTERICON:					// int	( const char* icon );
	case GT_USETARGETS:						// void ( const char* targetname );
		ret = SOF2GT_syscall(cmd, vmptr(args[0]));
		break;
	case GT_ATAN2:							// (double, double)
	case GT_DOESCLIENTHAVEITEM:				// bool ( int clientid, int itemid );
	case GT_ADDTEAMSCORE:					// void ( team_t team, int score );
	case GT_ADDCLIENTSCORE:					// void ( int clientid, int score );
	case GT_GIVECLIENTITEM:					// void ( int clientid, int itemid );
	case GT_TAKECLIENTITEM:					// void ( int clientid, int itemid );
	case GT_SETHUDICON:						// void	( int index, int icon );
		ret = SOF2GT_syscall(cmd, args[0], args[1]);
		break;
	case GT_TESTPRINTINT:					// (char*, int)
	case GT_TESTPRINTFLOAT:					// (char*, float)
		ret = SOF2GT_syscall(cmd, vmptr(args[0]), args[1]);
		break;
	case GT_TEXTMESSAGE:					// void ( int clientid, const char* message );
	case GT_RADIOMESSAGE:					// void ( int clientid, const char* message );
	case GT_GETCLIENTORIGIN:				// void ( int clientid, vec3_t origin );
	case GT_STARTSOUND:						// void ( int soundid, vec3_t origin );
		ret = SOF2GT_syscall(cmd, args[0], vmptr(args[1]));
		break;
	case GT_CVAR_SET:						// ( const char *var_name, const char *value );
	case GT_PERPENDICULARVECTOR:			// (vec3_t dst, const vec3_t src)
		ret = SOF2GT_syscall(cmd, vmptr(args[0]), vmptr(args[1]));
		break;
	case GT_MEMSET:							// (void* dest, int c, size_t count)
		ret = SOF2GT_syscall(cmd, vmptr(args[0]), args[1], args[2]);
		break;
	case GT_GETCLIENTNAME:					// void ( int clientid, const char* buffer, int buffersize );
	case GT_GETCLIENTITEMS:					// void ( int clientid, int* buffer, int buffersize );
	case GT_GETTRIGGERTARGET:				// void ( int triggerid, char* buffer, int buffersize );
	case GT_GETCLIENTLIST:					// int  ( team_t team, int* clients, int clientcount );
		ret = SOF2GT_syscall(cmd, args[0], vmptr(args[1]), args[2]);
		break;
	case GT_CVAR_VARIABLE_STRING_BUFFER:	// ( const char *var_name, char *buffer, int bufsize );
	case GT_MEMCPY:							// (void* dest, const void* src, size_t count)
	case GT_STRNCPY:						// (char* strDest, const char* strSource, size_t count)
		ret = SOF2GT_syscall(cmd, vmptr(args[0]), vmptr(args[1]), args[2]);
		break;
	case GT_REGISTERITEM:					// bool ( int itemid, const char* name, gtItemDef_t* def );
	case GT_REGISTERTRIGGER:				// bool ( int trigid, const char* name, gtTriggerDef_t* def );
	case GT_PLAYEFFECT:						// void	( int effect, vec3_t origin, vec3_t angles );
	case GT_SPAWNITEM:						// void ( int itemid, vec3_t origin, vec3_t angles );
		ret = SOF2GT_syscall(cmd, args[0], vmptr(args[1]), vmptr(args[2]));
		break;
	case GT_MATRIXMULTIPLY:					// (float in1[3][3], float in2[3][3], float out[3][3])
		ret = SOF2GT_syscall(cmd, vmptr(args[0]), vmptr(args[1]), vmptr(args[2]));
		break;
	case GT_CVAR_REGISTER:					// ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
		ret = SOF2GT_syscall(cmd, vmptr(args[0]), vmptr(args[1]), vmptr(args[2]), args[3]);
		break;
	case GT_ANGLEVECTORS:					// (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
		ret = SOF2GT_syscall(cmd, vmptr(args[0]), vmptr(args[1]), vmptr(args[2]), vmptr(args[3]));
		break;
	default:
		ret = 0;
	}

	return ret;
}


// entry point: handle gametype load from engine
C_DLLEXPORT void dllEntry(eng_syscall_t syscall) {
	gt_pluginvars.gt_syscall = syscall;

	QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "Gametype hook DLL loaded for gametype '%s'\n", gt_pluginvars.gt_gametype), QMMLOG_NOTICE);

	const char* modpath = QMM_VARARGS(PLID, "base/mp/qmm_gt_%sx86.dll", gt_pluginvars.gt_gametype);

	if (!s_load_dll(modpath)) {
		modpath = QMM_VARARGS(PLID, "vm/gt_%s.qvm", gt_pluginvars.gt_gametype);
		if (!s_load_qvm(modpath)) {
			g_shutdown = true;
			g_syscall(G_ERROR, QMM_VARARGS(PLID, "Could not locate DLL or QVM for gametype '%s'\n", gt_pluginvars.gt_gametype));
			return;
		}
	}
	QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "Successfully loaded %s for gametype '%s'\n", (gt_dll ? "DLL" : "QVM"), gt_pluginvars.gt_gametype), QMMLOG_NOTICE);
}


// attempt to load DLL gametype mod
static bool s_load_dll(const char* file) {
	mod_dllEntry_t gt_dllEntry;

	gt_dll = dlopen(file, RTLD_NOW);
	if (!gt_dll) {
		QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "s_load_dll(\"%s\"): Could not open DLL file for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		goto fail;
	}
	gt_dllEntry = (mod_dllEntry_t)dlsym(gt_dll, "dllEntry");
	if (!gt_dllEntry) {
		QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "s_load_dll(\"%s\"): Could not find 'dllEntry' in DLL for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		goto fail;
	}
	gt_pluginvars.gt_vmMain = (mod_vmMain_t)dlsym(gt_dll, "vmMain");
	if (!gt_pluginvars.gt_vmMain) {
		QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "s_load_dll(\"%s\"): Could not find 'vmMain' in DLL for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
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
	std::vector<uint8_t> filemem;
	int loaded;

	// load file using engine functions to read into pk3s if necessary
	filelen = g_syscall(G_FS_FOPEN_FILE, file, &fpk3, FS_READ);
	if (filelen <= 0) {
		QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "s_load_qvm(\"%s\"): Could not open QVM for reading for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		g_syscall(G_FS_FCLOSE_FILE, fpk3);
		return false;
	}
	filemem.resize((size_t)filelen);

	g_syscall(G_FS_READ, filemem.data(), filelen, fpk3);
	g_syscall(G_FS_FCLOSE_FILE, fpk3);

	// attempt to load mod
	loaded = qvm_load(&gt_qvm, filemem.data(), filemem.size(), SOF2GT_qvm_syscall, true, nullptr);
	if (!loaded) {
		QMM_WRITEQMMLOG(PLID, QMM_VARARGS(PLID, "s_load_qvm(\"%s\"): QVM load failed for gametype '%s'\n", file, gt_pluginvars.gt_gametype), QMMLOG_DEBUG);
		return false;
	}

	// special function to call into QVM
	gt_pluginvars.gt_vmMain = SOFT2GT_qvm_vmmain;

	return true;
}
