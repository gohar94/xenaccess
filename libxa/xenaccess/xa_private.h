/*
 * The libxa library provides access to resources in domU machines.
 * 
 * Copyright (C) 2005  Bryan D. Payne (bryan@thepaynes.cc)
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
 * $Id: xa_private.h 36 2006-11-30 01:38:20Z bdpayne $
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

/* offset to each of these fields from the beginning of the struct
   assuming that CONFIG_SCHEDSTATS is not defined  and CONFIG_KEYS
   is defined in the guest's kernel (this is the default in xen) */
#define XALINUX_TASKS_OFFSET 24 * 4   /* task_struct->tasks */
#define XALINUX_MM_OFFSET 30 * 4      /* task_struct->mm */
#define XALINUX_PID_OFFSET 39 * 4     /* task_struct->pid */
#define XALINUX_NAME_OFFSET 108 * 4   /* task_struct->comm */
#define XALINUX_PGD_OFFSET 9 * 4      /* mm_struct->pgd */
#define XALINUX_ADDR_OFFSET 32 * 4    /* mm_struct->pgd */


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

int xa_check_cache_sym (char *symbol_name, int pid, uint32_t *mach_address);

int xa_check_cache_virt (uint32_t virt_address, int pid,
                         uint32_t *mach_address);

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


/*TODO functions that still need comments */
int linux_system_map_symbol_to_address (
        xa_instance_t *instance, char *symbol, uint32_t *address);

void *linux_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset);

void *linux_access_machine_address (
        xa_instance_t *instance, uint32_t mach_address, uint32_t *offset);

void *linux_access_machine_address_rw (
        xa_instance_t *instance, uint32_t mach_address,
        uint32_t *offset, int prot);

void *linux_access_physical_address (
        xa_instance_t *instance, uint32_t phys_address, uint32_t *offset);

void *linux_access_user_virtual_address (
        xa_instance_t *instance, uint32_t virt_address,
        uint32_t *offset, int pid);

void *linux_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset);

char *linux_predict_sysmap_name (int id);

int xa_destroy_cache ();

char *xa_get_kernel_name (int id);

int xa_ishvm (int id);

#endif /* XA_PRIVATE_H */
