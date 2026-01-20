/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifdef __linux__

//#define _GNU_SOURCE
#include <link.h>
#include <dlfcn.h>
#include <stdio.h>
#include <elf.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/limits.h>

typedef void* (*pfndlopen)(const char*, int);

static char s_path[PATH_MAX];
static const char* s_gametype;
static pfndlopen old_dlopen = NULL;


// This is our dlopen hook. If the requested filename has "gt_", and the gametype string from G_GT_INIT,
// and does NOT have "qmm", then we need to return a pointer to this SO. However, without increasing the
// reference count of this SO, the first dlclose called on it to unload the gametype SO will also unload
// the plugin entirely, causing a crash in QMM. So to increase the reference count, we have to just
// change the path to this SO and call dlopen
static void* dlopen_hook(const char* path, int flags) {
    if (strstr(path, "gt_") &&
        strstr(path, s_gametype) &&
        !strstr(path, "qmm")) {
        return old_dlopen(s_path, flags);
    }
    return old_dlopen(path, flags);
}


#define ALIGN_ADDR(addr) ((void*)((uintptr_t)addr & ~(page_size - 1)))
void* install_hook(void* target_module, const char* function_name, void* function_hook) {
    size_t page_size = sysconf(_SC_PAGESIZE);   // need to store size of page for mprotect

    size_t funcnamelen = strlen(function_name);	// store length of function name for comparison (check first X bytes, see if next byte is '\0' or '@')

    struct link_map* lmap = NULL;		// from dlinfo man page: pointer to dynamic section of the shared object
    const char* plt_addr_base = NULL;	// from dlinfo man page: difference between an address in the ELF file and the address in memory

    const Elf32_Sym* dynsym = NULL;		// DT_SYMTAB: address of dynamic symbol table
    const char* dynstr = NULL;			// DT_STRTAB: dynamic string table
    size_t dynstr_size = 0;				// DT_STRSZ: total size in bytes of the dynamic string table (DT_STRTAB)
    const Elf32_Rel* rela_plt = NULL;	// DT_JMPREL: relocation entries
    size_t rela_plt_cnt = 0;			// DT_PLTRELSZ: total size in bytes of the relocation entries (DT_PLTRELSZ)

    const char* symbol_name;			// current name when looping through symbols
    void** symbol_addr;					// current address when looping through symbols

    void* ret = NULL;					// store and return old function address
    
    // get dynamic section of target module
    if (dlinfo(target_module, RTLD_DI_LINKMAP, &lmap) != 0) {
        printf("dlinfo failed!\n");
        return NULL;
    }

    plt_addr_base = (char*)lmap->l_addr;

    // go through all the sections and save various ones
    for (const Elf32_Dyn* dyn = lmap->l_ld; dyn->d_tag != DT_NULL; dyn++) {
        // .dynsym section
        if (dyn->d_tag == DT_SYMTAB) {
            dynsym = (const Elf32_Sym*)(dyn->d_un.d_ptr);
        }
        // check sizeof Elf32_Sym
        else if (dyn->d_tag == DT_SYMENT) {
            if (dyn->d_un.d_val != sizeof(Elf32_Sym)) {
                printf("DT_SYMENT wrong size\n");
                return NULL;
            }
        }
        // .dynstr section
        else if (dyn->d_tag == DT_STRTAB) {
            dynstr = (const char*)dyn->d_un.d_ptr;
        }
        // .dynstr size
        else if (dyn->d_tag == DT_STRSZ) {
            dynstr_size = dyn->d_un.d_val;
        }
        // .rela.plt or .rel.plt section
        else if (dyn->d_tag == DT_JMPREL) {
            rela_plt = (const Elf32_Rel*)(dyn->d_un.d_ptr);
        }
        // .rela.plt or .rel.plt section size
        else if (dyn->d_tag == DT_PLTRELSZ) {
            rela_plt_cnt = dyn->d_un.d_val / sizeof(Elf32_Rel);
        }
    }
    // make sure we got everything we need
    if (!dynsym || !dynstr || !dynstr_size || !rela_plt || !rela_plt_cnt)
        return NULL;

    // loop through all relocations
    for (const Elf32_Rel* plt = rela_plt; plt != rela_plt + rela_plt_cnt; plt++) {
        // low byte of r_info is the type, high 3 bytes of r_info is the symbol index
        if (ELF32_R_TYPE(plt->r_info) == R_386_JMP_SLOT) {
            size_t idx = ELF32_R_SYM(plt->r_info);
            // get string index for symbol
            idx = dynsym[idx].st_name;
            // string index out of bounds, skip
            if (idx + 1 > dynstr_size)
                continue;
            symbol_name = dynstr + idx;
            symbol_addr = (void**)(plt_addr_base + plt->r_offset);
            // if this symbol matches the given function name
            if (!strncmp(symbol_name, function_name, funcnamelen) && (symbol_name[funcnamelen] == '\0' || symbol_name[funcnamelen] == '@')) {
                if (mprotect(ALIGN_ADDR(symbol_addr), page_size, PROT_READ | PROT_WRITE) != 0)
                    return NULL;
                ret = *symbol_addr;
                *symbol_addr = function_hook;
                // mprotect(ALIGN_ADDR(symbol_addr), page_size, PROT_READ | PROT_EXEC);
                return ret;
            }
        }
    }

    return NULL;
}


static void* s_proc_handle = nullptr;
bool hook_enable(const char* gametype) {
    s_gametype = gametype;

    static char path[PATH_MAX];
    // get our file path
    Dl_info dli;
    memset(&dli, 0, sizeof(dli));
    if (!dladdr(&dli, &dli))
        return false;
    strncpy(s_path, dli.dli_sname, sizeof(s_path));
    s_path[sizeof(s_path) - 1] = '\0';

    // get handle to main executable
    s_proc_handle = dlopen(NULL, RTLD_NOLOAD);

    // install hook
    old_dlopen = (pfndlopen)install_hook(s_proc_handle, "dlopen", (void*)dlopen_hook);
    return !!old_dlopen;
}


bool hook_disable() {
    return !!install_hook(s_proc_handle, "dlopen", (void*)old_dlopen);
}

#endif // __linux__
