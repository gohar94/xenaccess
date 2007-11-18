/*
 * The libxa library provides access to resources in domU machines.
 * 
 * Copyright (C) 2005 - 2007  Bryan D. Payne (bryan@thepaynes.cc)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * --------------------
 * This file contains the public function definitions for the libxa
 * library.
 *
 * File: xenaccess.h
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 */

/**
 * @file xenaccess.h
 * @brief The primary XenAccess API is defined here.
 *
 * More detailed description can go here.
 */
#ifndef LIB_XEN_ACCESS_H
#define LIB_XEN_ACCESS_H

#include <xenctrl.h>

/* uncomment this to enable debug output */
//#define XA_DEBUG

/**
 * Return value indicating success.
 */
#define XA_SUCCESS 0
/**
 * Return value indicating failure.
 */
#define XA_FAILURE -1
/**
 * Constant used to specify Linux in the os_type member of the
 * xa_instance struct.
 */
#define XA_OS_LINUX 0
/**
 * Constant used to specify Windows in the os_type member of the
 * xa_instance struct.
 */
#define XA_OS_WINDOWS 1

/*TODO find a way to move this to xa_private.h */
struct xa_cache_entry{
    time_t last_used;
    char *symbol_name;
    uint32_t virt_address;
    uint32_t mach_address;
    int pid;
    struct xa_cache_entry *next;
    struct xa_cache_entry *prev;
};
typedef struct xa_cache_entry* xa_cache_entry_t;

/**
 * @brief XenAccess instance.
 *
 * This struct holds all of the relavent information for an instance of
 * XenAccess.  Each time a new domain is accessed, a new instance must
 * be created using the xa_init function.  When you are done with an instance,
 * its resources can be freed using the xa_destroy function.
 */
typedef struct xa_instance{
    int xc_handle;          /**< handle to xenctrl library (libxc) */
    uint32_t domain_id;     /**< domid that we are accessing */
    char *domain_name;      /**< domain name that we are accessing */
    char *sysmap;           /**< system map file for domain's running kernel */
    uint32_t page_offset;   /**< page offset for this instance */
    uint32_t page_shift;    /**< page shift for last mapped page */
    uint32_t page_size;     /**< page size for last mapped page */
    uint32_t kpgd;          /**< kernel page global directory */
    uint32_t init_task;     /**< address of task struct for init */
    uint32_t ntoskrnl;      /**< base physical address for ntoskrnl image */
    int os_type;            /**< type of os: XA_OS_LINUX, etc */
    int hvm;                /**< nonzero if HVM domain */
    int pae;                /**< nonzero if PAE is enabled */
    int pse;                /**< nonzero if PSE is enabled */
    xc_dominfo_t info;      /**< libxc info: domid, ssidref, stats, etc */
    unsigned long *live_pfn_to_mfn_table;
    unsigned long nr_pfns;
    xa_cache_entry_t cache_head;
    xa_cache_entry_t cache_tail;
    int current_cache_size;
} xa_instance_t;

/**
 * @brief Linux task addresses.
 *
 * This struct holds the task addresses that are found in a task's
 * memory descriptor.  You can fill the values in the struct using
 * the xa_linux_get_taskaddr function.  The comments next to each
 * entry are taken from Bovet & Cesati's excellent book Understanding
 * the Linux Kernel 3rd Ed, p354.
 */
typedef struct xa_linux_taskaddr{
    unsigned long start_code;  /**< initial address of executable code */
    unsigned long end_code;    /**< final address of executable code */
    unsigned long start_data;  /**< initial address of initialized data */
    unsigned long end_data;    /**< final address of initialized data */
    unsigned long start_brk;   /**< initial address of the heap */
    unsigned long brk;         /**< current final address of the heap */
    unsigned long start_stack; /**< initial address of user mode stack */
    unsigned long arg_stack;   /**< initial address of command-line arguments */
    unsigned long arg_end;     /**< final address of command-line arguments */
    unsigned long env_start;   /**< initial address of environmental vars */
    unsigned long env_end;     /**< final address of environmental vars */
} xa_linux_taskaddr_t;

/**
 * @brief Windows PEB information.
 *
 * This struct holds process information found in the PEB, which is 
 * part of the EPROCESS structure.  You can fill the values in the
 * struct using the xa_windows_get_peb function.  Note that this
 * struct does not contain all information from the PEB.
 */
typedef struct xa_windows_peb{
    uint32_t ImageBaseAddress; /**< initial address of executable code */
    uint32_t ProcessHeap;      /**< initial address of the heap */
} xa_windows_peb_t;

/*--------------------------------------------------------
 * Initialization and Destruction functions from xa_core.c
 */

/**
 * Initializes access to a specific domU given a domain id.  The
 * domain id must represent an active domain and must be > 0.  All
 * calls to xa_init must eventually call xa_destroy.
 *
 * This is a costly funtion in terms of the time needed to execute.
 * You should call this function only once per domain, and then use the
 * resulting instance when calling any of the other library functions.
 *
 * @param[in] domain_id Domain id to access, specified as a number
 * @param[out] instance Struct that holds instance information
 * @return XA_SUCCESS on success, XA_FAILURE on failure
 */
int xa_init (uint32_t domain_id, xa_instance_t *instance);

/**
 * Destroys an instance by freeing memory and closing any open handles.
 *
 * @param[in] instance Instance to destroy
 * @return XA_SUCCESS on success, XA_FAILURE on failure
 */
int xa_destroy (xa_instance_t *instance);

/*-----------------------------------------
 * Memory access functions from xa_memory.c
 */

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with a kernel symbol (e.g.,
 * from System.map on linux).  This memory must be unmapped manually
 * with munmap.
 *
 * @param[in] instance XenAccess instance
 * @param[in] symbol Desired kernel symbol to access
 * @param[out] offset Offset to kernel symbol within the mapped memory
 * @return Beginning of mapped memory page or NULL on error
 */
void *xa_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset);

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with a kernel virtual address.
 * This memory must be unmapped manually with munmap.
 *
 * @param[in] instance XenAccess instance
 * @param[in] virt_address Virtual address to access
 * @param[out] offset Offset to address within the mapped memory
 * @return Beginning of mapped memory page or NULL on error
 */
void *xa_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset);

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with a virtual address from a 
 * process' address space.  This memory must be unmapped manually
 * with munmap.
 *
 * @param[in] instance XenAccess instance
 * @param[in] virt_address Desired virtual address to access
 * @param[out] offset Offset to address within the mapped memory
 * @param[in] pid PID of process' address space to use.  If you specify
 *     0 here, XenAccess will access the kernel virtual address space and
 *     this function's behavior will be the same as xa_access_virtual_address.
 * @return Beginning of mapped memory page or NULL on error
 */
void *xa_access_user_virtual_address (
        xa_instance_t *instance, uint32_t virt_address,
        uint32_t *offset, int pid);

/**
 * Reads a long (32 bit) value from memory, given a kernel symbol.
 *
 * @param[in] instance XenAccess instance
 * @param[in] sym Kernel symbol to read from
 * @return Value from memory, or zero on error
 */
uint32_t xa_read_long_sym (xa_instance_t *instance, char *sym);

/**
 * Reads a long long (64 bit) value from memory, given a kernel symbol.
 *
 * @param[in] instance XenAccess instance
 * @param[in] sym Kernel symbol to read from
 * @return Value from memory, or zero on error
 */
uint64_t xa_read_long_long_sym (xa_instance_t *instance, char *sym);

/**
 * Reads a long (32 bit) value from memory, given a virtual address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] vaddr Virtual address to read from
 * @param[in] pid Pid of the virtual address space (0 for kernel)
 * @return Value from memory, or zero on error
 */
uint32_t xa_read_long_virt (xa_instance_t *instance, uint32_t vaddr, int pid);

/**
 * Reads a long long (64 bit) value from memory, given a virtual address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] vaddr Virtual address to read from
 * @param[in] pid Pid of the virtual address space (0 for kernel)
 * @return Value from memory, or zero on error
 */
uint64_t xa_read_long_long_virt (
        xa_instance_t *instance, uint32_t vaddr, int pid);

/**
 * Reads a long (32 bit) value from memory, given a physical address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] paddr Physical address to read from
 * @return Value from memory, or zero on error
 */
uint32_t xa_read_long_phys (xa_instance_t *instance, uint32_t paddr);

/**
 * Reads a long long (64 bit) value from memory, given a physical address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] paddr Physical address to read from
 * @return Value from memory, or zero on error
 */
uint64_t xa_read_long_long_phys (xa_instance_t *instance, uint32_t paddr);

/**
 * Reads a long (32 bit) value from memory, given a machine address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] maddr Machine address to read from
 * @return Value from memory, or zero on error
 */
uint32_t xa_read_long_mach (xa_instance_t *instance, uint32_t maddr);

/**
 * Reads a long long (64 bit) value from memory, given a machine address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] maddr Machine address to read from
 * @return Value from memory, or zero on error
 */
uint64_t xa_read_long_long_mach (xa_instance_t *instance, uint32_t maddr);

/**
 * Performs the translation from a kernel virtual address to a
 * physical address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] virt_address Desired kernel virtual address to translate
 * @return Physical address, or zero on error
 */
uint32_t xa_translate_kv2p(xa_instance_t *instance, uint32_t virt_address);

/*-----------------------------
 * Linux-specific functionality
 */

/**
 *
 * @param[in] instance XenAccess instance
 * @param[in] pid
 * @param[out] taskaddr
 * @return
 */
int xa_linux_get_taskaddr (
        xa_instance_t *instance, int pid, xa_linux_taskaddr_t *taskaddr);

/*-----------------------------
 * Windows-specific functionality
 */

/**
 *
 * @param[in] instance XenAccess instance
 * @param[in] pid
 * @param[out] peb
 * @return
 */
int xa_windows_get_peb (
        xa_instance_t *instance, int pid, xa_windows_peb_t *peb);


#endif /* LIB_XEN_ACCESS_H */

/**
 * @mainpage
 *
 * Main page description goes here.
 */
