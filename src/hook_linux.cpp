/*
SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
Copyright 2025-2026
https://github.com/thecybermind/sof2gt_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifdef __linux__

#define _GNU_SOURCE
#include <link.h>
#include <dlfcn.h>
#include <stdio.h>
#include <elf.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define ALIGN_ADDR(addr) ((void*)((uintptr_t)addr & s_page_mask))

typedef void* (*pfndlopen)(const char*, int);

static size_t s_page_mask;
static char s_path[MAX_PATH];
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
    return old_dlopen(path);
}


int plthook_enum(unsigned int* pos, const char** name_out, void*** addr_out) {
    const Elf32_Rel* plt = NULL;
    while (*pos < plthook.rela_plt_cnt) {
        plt = plthook.rela_plt + *pos;
        int rv = -1;
        if (ELF32_R_TYPE(plt->r_info) == R_386_JMP_SLOT) {
            size_t idx = ELF32_R_SYM(plt->r_info);
            idx = plthook.dynsym[idx].st_name;
            if (idx + 1 > plthook.dynstr_size)
                rv = -1;
            else {
                *name_out = plthook.dynstr + idx;
                *addr_out = (void**)(plthook.plt_addr_base + plt->r_offset);
                rv = 0;
            }
        }
        (*pos)++;
        if (rv >= 0)
            return rv;
    }
    *name_out = NULL;
    *addr_out = NULL;
    return -1;
}


void* install_hook(void* target_module, const char* function_name, void* function_hook) {
    size_t funcnamelen = strlen(function_name);	// store length of function name for comparison (check first X bytes, see if next byte is '\0' or '@')

    struct link_map* lmap = NULL;		// from dlinfo man page: pointer to dynamic section of the shared object
    const char* plt_addr_base = NULL;	// from dlinfo man page: difference between the address in the ELF file and the address in memory
    const Elf32_Dyn* dyn = NULL;		// loop through each dynamic section and store the following values:
    const Elf32_Sym* dynsym = NULL;		// DT_SYMTAB: address of dynamic symbol table
    const char* dynstr = NULL;			// DT_STRTAB: dynamic string table
    size_t dynstr_size = 0;				// DT_STRSZ: total size in bytes of the dynamic string table (DT_STRTAB)
    const Elf32_Rel* rela_plt = NULL;	// DT_JMPREL: relocation entries
    size_t rela_plt_cnt = 0;			// DT_PLTRELSZ: total size in bytes of the relocation entries (DT_PLTRELSZ)

    unsigned int pos = 0;
    const char* name;
    void** addr;
    int rv;
    void* ret = NULL;					// store and return old function address

    // get page size and set mask for aligning addr for mprotect
    if (page_mask == 0) {
        page_mask = ~(sysconf(_SC_PAGESIZE) - 1);
    }

    // get dynamic section of target module
    if (dlinfo(target_module, RTLD_DI_LINKMAP, &lmap) != 0) {
        printf("dlinfo failed!\n");
        return NULL;
    }

    plt_addr_base = (char*)lmap->l_addr;

    // go through all the sections and save various ones
    dyn = lmap->l_ld;
    for (dyn = lmap->l_ld; dyn->d_tag != DT_NULL; dyn++) {
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
            dynstr = dyn->d_un.d_ptr;
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
    if (!dynsym || !dynstr || !dynstr_size || !rela_plt || !rela_plt_cnt) {
        printf("could not find certain tags\n");
        return NULL;
    }

    // todo: merge in plthook_enum 
    while ((rv = plthook_enum(&pos, &name, &addr)) == 0) {
        printf("import found: %s %p @ %p\n", name, *addr, addr);
        if (!strncmp(name, function_name, funcnamelen) && (name[funcnamelen] == '\0' || name[funcnamelen] == '@')) {
            printf("matching import found for %s: %p\n", function_name, *addr);
            if (mprotect(ALIGN_ADDR(addr), page_size, PROT_READ | PROT_WRITE) != 0) {
                printf("could not mprotect RW\n");
                return NULL;
            }
            ret = *addr;
            *addr = function_hook;
            printf("import entry for %s is now: %p\n", function_name, *addr);
            // mprotect(ALIGN_ADDR(addr), page_size, PROT_READ | PROT_EXEC);
            return ret;
        }
    }

    return NULL;
}


static void* s_proc_handle = nullptr;
bool hook_enable(const char* gametype) {
    s_gametype = gametype;

    // get our file path
    Dl_info dli;
    if (!dladdr(&dli, &dli))
        return false;
    strncpy(s_path, dli.dli_sname, sizeof(s_path));
    s_path[sizeof(s_path) - 1] = '\0';

    // get handle to main executable
    s_proc_handle = dlopen(NULL, RTLD_NOW);
    dlclose(s_proc_handle);

    // install hook
    old_dlopen = (pfndlopen_t)install_hook(s_proc_handle, "dlopen", dlopen_hook);
    return !!old_dlopen;
}


bool hook_disable() {
    return !!install_hook(s_proc_handle, "dlopen", old_dlopen);
}

#endif // __linux__
