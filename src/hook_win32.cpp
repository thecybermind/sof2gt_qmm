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
#include <direct.h>

#include "main.h"

typedef HMODULE(WINAPI *pfnLLA_t)(LPCSTR);

static void* install_hook(HMODULE target, const char* dllname, const char* functionname, void* functionhook);
static HMODULE s_dll = nullptr;
static const char* s_gametype;

// just store our module pointer
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID) {
    s_dll = hinstDLL;
    return TRUE;
}

// this is our LoadLibrary hook. if the requested filename has "gt_", the gametype string from G_GT_INIT,
// and does NOT have "qmm", then point to this DLL. otherwise use normal LoadLibrary
pfnLLA_t pfnLoadLibraryA = nullptr;
HMODULE WINAPI LoadLibraryA_Hook(LPCSTR lpLibFileName) {
    if (strstr(lpLibFileName, "gt_") &&
        strstr(lpLibFileName, s_gametype) &&
        !strstr(lpLibFileName, "qmm"))
        return s_dll;
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
    PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_NT_HEADERS peHeader;

    PIMAGE_OPTIONAL_HEADER optionalHeader;
    PIMAGE_DATA_DIRECTORY importDirectory;

    PIMAGE_IMPORT_DESCRIPTOR importDescriptor;

    // start looking at the module as a DOS image
    dosHeader = (PIMAGE_DOS_HEADER)target_module;

    // verify DOS magic number
    if (((*dosHeader).e_magic) != IMAGE_DOS_SIGNATURE)
        return nullptr;

    // get PE header
    peHeader = (PIMAGE_NT_HEADERS)(dosHeader->e_lfanew + (intptr_t)target_module);

    // verify PE magic number
    if (peHeader->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    // get optional header
    optionalHeader = &peHeader->OptionalHeader;

    // verify optional header magic number
    if (optionalHeader->Magic != 0x10B)
        return nullptr;

    // now we parse through its import descriptors
    importDirectory = &optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    // get import descriptor table
    importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(importDirectory->VirtualAddress + (intptr_t)target_module);

    // loop through each descriptor table looking for a matching DLL name
    for (int index = 0; importDescriptor[index].Characteristics != 0; index++) {
        PIMAGE_THUNK_DATA thunkILT;
        PIMAGE_THUNK_DATA thunkIAT;
        PIMAGE_IMPORT_BY_NAME nameData;

        // get the dll name field and if not null, add base address
        const char* importedDLLName = (const char*)(importDescriptor[index].Name);
        if (!importedDLLName)
            return nullptr;
        importedDLLName += (intptr_t)target_module;

        // if this dll isn't the one we're looking for (NULL = all), continue 
        if (dll_name && _stricmp(importedDLLName, dll_name) != 0)
            continue;

        // analyzeImportDescriptor(importDescriptor[index], peHeader, baseAddress, nameOfAPI);

        // get the RVAs of OriginalFirstThunk & FirstThunk and if not null, add base address
        thunkILT = (PIMAGE_THUNK_DATA)(importDescriptor[index].OriginalFirstThunk);
        thunkIAT = (PIMAGE_THUNK_DATA)(importDescriptor[index].FirstThunk);
        if (!thunkILT || !thunkIAT)
            return nullptr;
        thunkILT = (PIMAGE_THUNK_DATA)((intptr_t)thunkILT + (intptr_t)target_module);
        thunkIAT = (PIMAGE_THUNK_DATA)((intptr_t)thunkIAT + (intptr_t)target_module);

        // loop as long as RVA of imported name (AddressOfData) is not 0
        for (; thunkILT->u1.AddressOfData != 0; thunkILT++, thunkIAT++) {
            // store old function address
            void* ret = nullptr;

            // skip if the function was imported by ordinal number
            if ((thunkILT->u1.Ordinal & IMAGE_ORDINAL_FLAG) == IMAGE_ORDINAL_FLAG)
                continue;

            // if this function's name matches what we want
            nameData = (PIMAGE_IMPORT_BY_NAME)(thunkILT->u1.AddressOfData + (intptr_t)target_module);
            if (!strcmp(function_name, (char*)nameData->Name)) {
                // adjust memory permissions
                MEMORY_BASIC_INFORMATION thunkMemInfo;
                DWORD oldProtect, _;

                // get old function address
                ret = (void*)thunkIAT->u1.Function;
                
                if (!VirtualQuery(&thunkIAT->u1.Function, &thunkMemInfo, sizeof(thunkMemInfo)))
                    return nullptr;

                if (!VirtualProtect(thunkMemInfo.BaseAddress, thunkMemInfo.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect))
                    return nullptr;

                // replace function with the hook
                thunkIAT->u1.Function = (intptr_t)function_hook;

                VirtualProtect(thunkMemInfo.BaseAddress, thunkMemInfo.RegionSize, oldProtect, &_);

                return ret;
            }
        } // thunkILT loop
    } // importDescriptor[index] loop

    return nullptr;
}

#endif // _WIN32
