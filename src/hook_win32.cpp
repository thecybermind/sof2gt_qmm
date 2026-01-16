/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifdef _WIN32

#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>

#include "main.h"

#pragma comment( lib, "Dbghelp.lib") // ImageDirectoryEntryToData

typedef HMODULE(WINAPI *pfnLLA_t)(LPCSTR);

static void* install_hook(HMODULE target, const char* dllname, const char* functionname, void* functionhook);
static HMODULE s_dll = nullptr;
static const char* s_gametype;

// just store our module pointer
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID) {
    s_dll = hinstDLL;
    return TRUE;
}


// This is our LoadLibrary hook. If the requested filename has "gt_", and the gametype string from G_GT_INIT,
// and does NOT have "qmm", then we need to return a pointer to this DLL (s_dll). However, without increasing the
// reference count of this DLL, the first FreeLibrary called on it to unload the gametype DLL will also unload the
// plugin entirely, causing a crash in QMM. So to increase the reference count, we use GetModuleHandleExA.
pfnLLA_t pfnLoadLibraryA = nullptr;
HMODULE WINAPI LoadLibraryA_Hook(LPCSTR lpLibFileName) {
    if (strstr(lpLibFileName, "gt_") &&
        strstr(lpLibFileName, s_gametype) &&
        !strstr(lpLibFileName, "qmm")) {
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)s_dll, &s_dll);
        return s_dll;
    }
    return pfnLoadLibraryA(lpLibFileName);
}


bool hook_enable(const char* gametype) {
    s_gametype = gametype;
    pfnLoadLibraryA = (pfnLLA_t)install_hook(GetModuleHandle(NULL), "kernel32.dll", "LoadLibraryA", LoadLibraryA_Hook);
    return !!pfnLoadLibraryA;
}


bool hook_disable() {
    return install_hook(GetModuleHandle(NULL), "kernel32.dll", "LoadLibraryA", pfnLoadLibraryA);
}


static void* install_hook(HMODULE target_module, const char* dll_name, const char* function_name, void* function_hook) {
    PIMAGE_IMPORT_DESCRIPTOR importDescriptorTable, importDescriptor;
    char* module_base = (char*)target_module;   // module base as a char* for adding RVAs
    void* ret = nullptr;                        // store old function address
    MEMORY_BASIC_INFORMATION thunkMemInfo;
    DWORD oldProtect, _;

    // get import descriptor table
    importDescriptorTable = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(target_module, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &_);

    // loop through each descriptor entry looking for a matching DLL name (NULL = check every DLL)
    for (importDescriptor = importDescriptorTable; importDescriptor->Name != 0; importDescriptor++) {
        PIMAGE_THUNK_DATA thunk_name = (PIMAGE_THUNK_DATA)(module_base + importDescriptor->OriginalFirstThunk);
        PIMAGE_THUNK_DATA thunk_addr = (PIMAGE_THUNK_DATA)(module_base + importDescriptor->FirstThunk);

        // if this dll isn't the one we're looking for, continue 
        if (dll_name && _stricmp(module_base + importDescriptor->Name, dll_name) != 0)
            continue;

        // loop as long as RVA of imported name (AddressOfData) is not 0
        for (; thunk_addr->u1.Function != 0; thunk_name++, thunk_addr++) {
            // skip if the function was imported by ordinal number
            if ((thunk_name->u1.Ordinal & IMAGE_ORDINAL_FLAG) == IMAGE_ORDINAL_FLAG)
                continue;

            // if this function's name matches what we want
            if (!strcmp(function_name, ((PIMAGE_IMPORT_BY_NAME)(module_base + thunk_name->u1.AddressOfData))->Name)) {
                // get old function address
                ret = (void*)thunk_addr->u1.Function;
                
                // get memory page info for our desired thunk
                if (!VirtualQuery(&thunk_addr->u1.Function, &thunkMemInfo, sizeof(thunkMemInfo)))
                    return nullptr;

                // allow writing to the thunk page
                if (!VirtualProtect(thunkMemInfo.BaseAddress, thunkMemInfo.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect))
                    return nullptr;

                // replace function with the hook
                thunk_addr->u1.Function = (intptr_t)function_hook;

                // restore previous permissions to thunk page
                VirtualProtect(thunkMemInfo.BaseAddress, thunkMemInfo.RegionSize, oldProtect, &_);

                // return old function address
                return ret;
            }
        } // thunk loop
    } // importDescriptor loop

    return nullptr;
}

#endif // _WIN32
