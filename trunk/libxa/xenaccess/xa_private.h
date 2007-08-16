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
 * This file contains function headers for items that are not exported
 * outside of the library for public use.  These functions are intended
 * to only be used inside the library.
 *
 * File: xa_private.h
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date: 2006-11-29 20:38:20 -0500 (Wed, 29 Nov 2006) $
 */
#ifndef XA_PRIVATE_H
#define XA_PRIVATE_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <xenctrl.h>
#include "xenaccess.h"

/* Architecture dependent constants */
#define XA_PFN_PER_FRAME 1024		/* number of xen_pfn_t that fits on one frame */

/* offset to each of these fields from the beginning of the struct
   assuming that CONFIG_SCHEDSTATS is not defined  and CONFIG_KEYS
   is defined in the guest's kernel (this is the default in xen) */
#define XALINUX_TASKS_OFFSET 24 * 4   /* task_struct->tasks */
#define XALINUX_MM_OFFSET 30 * 4      /* task_struct->mm */
#define XALINUX_PID_OFFSET 39 * 4     /* task_struct->pid */
#define XALINUX_NAME_OFFSET 108 * 4   /* task_struct->comm */
#define XALINUX_PGD_OFFSET 9 * 4      /* mm_struct->pgd */
#define XALINUX_ADDR_OFFSET 32 * 4    /* mm_struct->pgd */


/*------------------------------
 * Utility function from xa_util
 */

/**
 * Get the specifid bit from a given register entry.
 *
 * @param[in] reg The register contents to parse (e.g., CR0, CR3, etc)
 * @param[in] bit The number of the bit to check.
 * @param[out] zero if the bit in question is zero, else one
 */
int xa_get_bit (unsigned long reg, int bit);

/**
 * Typical debug print function.  Only produces output when XA_DEBUG is
 * defined (usually in xenaccess.h) at compile time.
 */
#ifndef XA_DEBUG
#define xa_dbprint(format, args...) ((void)0)
#else
void xa_dbprint(char *format, ...);
#endif

/*-------------------------------------
 * Definitions to support the LRU cache
 */
#define XA_CACHE_SIZE 10

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
 * Check if \a symbol_name is in the LRU cache.
 *
 * @param[in] symbol_name Name of the requested symbol.
 * @param[in] pid Id of the associated process.
 * @param[out] mach_address Machine address of the symbol.
 */
int xa_check_cache_sym (char *symbol_name, int pid, uint32_t *mach_address);


/**
 * Check if \a virt_address is in the LRU cache.
 * 
 * @param[in] virt_address Virtual address in space of guest process.
 * @param[in] pid Id of the process.
 * @param[out] mach_address Machine address of the symbol.
 */
int xa_check_cache_virt (uint32_t virt_address, int pid,
                         uint32_t *mach_address);

/**
 * Updates cache of guest symbols. Every symbol name has an 
 * associated virtual address (address space of host process),
 * pid and machine address (see memory chapter in Xen developers doc).
 *
 * @param[in] symbol_name Name of the cached symbol
 * @param[in] virt_address Virtual address of the symbol
 * @param[in] pid Id of the process associated with symbol
 * @param[in] mach_address Machine address
 */
int xa_update_cache (char *symbol_name, uint32_t virt_address,
                     int pid, uint32_t mach_address);


/*--------------------------------------------
 * Print util functions from xa_pretty_print.c
 */

/**
 * Prints out the hex and ascii version of a chunk of bytes. The
 * output is similar to what you would get with 'od -h' with the
 * additional ascii information on the right side of the display.
 *
 * @param[in] data The bytes that will be printed to stdout
 * @param[in] length The length (in bytes) of data
 */
void print_hex (unsigned char *data, int length);

/**
 * Prints out the data in a xc_dominfo_t struct to stdout.
 *
 * @param[in] info The struct to print
 */
void print_dominfo (xc_dominfo_t info);

/*-----------------------------------------
 * Memory access functions from xa_memory.c
 */

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with the machine frame number.
 * This memory must be unmapped manually with munmap.
 *
 * @param[in] instance libxa instance
 * @param[in] prot Desired memory protection (see 'man mmap' for values)
 * @param[in] mfn Machine frame number
 * @return Mapped memory or NULL on error
 */
void *xa_mmap_mfn (xa_instance_t *instance, int prot, unsigned long mfn);

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with the page frame number.
 * This memory must be unmapped manually with munmap.
 *
 * @param[in] instance libxa instance
 * @param[in] prot Desired memory protection (see 'man mmap' for values)
 * @param[in] pfn Page frame number
 * @return Mapped memory or NULL on error
 */
void *xa_mmap_pfn (xa_instance_t *instance, int prot, unsigned long pfn);

/**
 * Memory maps page in domU that contains given physical address.
 * The mapped memory is read-only.
 *
 * @param[in] instance Handle to xenaccess instance.
 * @param[in] phys_address Requested physical address.
 * @param[out] offset Offset of the address in returned page.
 *
 * @return Address of a page copy that contains phys_address.
 */
void *xa_access_physical_address (
        xa_instance_t *instance, uint32_t phys_address, uint32_t *offset);

/**
 * Memory maps page in domU that contains given machine address. For more
 * info about machine, virtual and pseudo-physical page see xen dev docs.
 *
 * @param[in] instance Handle to xenaccess instance.
 * @param[in] mach_address Requested machine address.
 * @param[out] offset Offset of the address in returned page.
 *
 * @return Address of a page copy with content like mach_address.
 */
void *xa_access_machine_address (
        xa_instance_t *instance, uint32_t mach_address, uint32_t *offset);

/**
 * Memory maps page in domU that contains given machine address. Allows
 * caller to specify r/w access.
 *
 * @param[in] instance Handle to xenaccess instance.
 * @param[in] mach_address Requested machine address.
 * @param[out] offset Offset of the address in returned page.
 * @param[in] prot Desired memory protection (see 'man mmap' for values).
 *
 * @return Address of a page copy with content like mach_address.
 */
void *xa_access_machine_address_rw (
        xa_instance_t *instance, uint32_t mach_address,
        uint32_t *offset, int prot);

/**
 * Covert virtual address to machine address via page table lookup.
 *
 * @param[in] instance Handle to xenaccess instance.
 * @param[in] pgd Page directory to use for this lookup.
 * @param[in] virt_address Virtual address to convert.
 * @param[in] kernel 0 for user space lookup, 1 for kernel lookup
 *
 * @return Machine address resulting from page table lookup.
 */
uint32_t xa_pagetable_lookup (
            xa_instance_t *instance, uint32_t pgd,
            uint32_t virt_address, int kernel);

/** 
 * Memory maps page in domU that contains given virtual address
 * and belongs to process \a pid. 
 *
 * @param[in] instance Handle to xenaccess instance.
 * @param[in] virt_address Requested virtual address.
 * @param[out] offset Offset of the address in returned page.
 * @param[in] pid Id of the process in which the virt_address is.
 *
 * @return Address of a page copy that contains virt_address.
 */
void *xa_access_user_virtual_address (
        xa_instance_t *instance, uint32_t virt_address,
        uint32_t *offset, int pid);

/**
 * Memory maps page in domU that contains given virtual address.
 * The func is mainly useful for accessing kernel memory.
 *
 * @param[in] instance Handle to xenaccess instance.
 * @param[in] virt_address Requested virtual address.
 * @param[out] offset Offset of the address in returned page.
 *
 * @return Address of a page copy that contains virt_address.
 */
void *xa_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset);

/**
 * Find the address of the page global directory for a given PID
 *
 * @param[in] instance Handle to xenaccess instance.
 * @param[in] pid The process to lookup.
 *
 * @return Address of pgd, or zero if no address could be found.
 */
uint32_t xa_pid_to_pgd (xa_instance_t *instance, int pid);

/**
 * Gets address of a symbol in domU virtual memory. It uses System.map
 * file specified in xenaccess configuration file.
 *
 * @param[in] instance Handle to xenaccess instance (see xa_init).
 * @param[in] symbol Name of the requested symbol.
 * @param[out] address The addres of the symbol in guest memory.
 */
int linux_system_map_symbol_to_address (
        xa_instance_t *instance, char *symbol, uint32_t *address);

/**
 * Gets a memory page where \a symbol is located and sets \a offset
 * of the symbol. The mapping is cached internally. (TODO: should the caller
 * munmap as in xa_access_virtual_address or the cache management does so)
 *
 * @param[in] instance Handle to xenaccess instance.
 * @param[in] symbol Name of the requested symbol.
 * @param[out] offset Offset of symbol in returned page.
 *
 * @return Address of a page where \a symbol resides.
 */
void *linux_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset);

/**
 * \deprecated Tries to guess the right System.map file for the
 * domU kernel. The configuration option is now preferred.
 * 
 * @param[in] id Domain id.
 *
 * @return String with the path for the System.map (must be freed by caller). 
 */
char *linux_predict_sysmap_name (int id);

/**
 * Releases the cache.
 *
 * @return 0 for success. -1 for failure.
 */
int xa_destroy_cache ();

/**
 * Gets name of the kernel for given \a id.
 *
 * @param[in] id Domain id.
 *
 * @return String with the path to domU kernel.
 */
char *xa_get_kernel_name (int id);

/**
 * Finds out whether the domU is HVM (Hardware virtual machine).
 *
 * @param[in] id Domain id.
 *
 * @return 1 if domain is HVM. 0 otherwise.
 */
int xa_ishvm (int id);

#endif /* XA_PRIVATE_H */
