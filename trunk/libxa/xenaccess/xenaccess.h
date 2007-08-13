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
 *
 * $Id$
 * $Date: 2006-12-06 01:23:30 -0500 (Wed, 06 Dec 2006) $
 */
#ifndef LIB_XEN_ACCESS_H
#define LIB_XEN_ACCESS_H

#include <xenctrl.h>

/* uncomment this to enable debug output */
#define XA_DEBUG

#define XA_SUCCESS 0
#define XA_FAILURE -1
#define XA_OS_LINUX 0
#define XA_OS_WINDOWS 1  /* not yet supported */
#define XA_OS_NETBSD 2   /* not yet supported */
#define XA_PAGE_SIZE XC_PAGE_SIZE
#define XA_PAGE_OFFSET 0xc0000000

typedef struct xa_instance{
    int xc_handle;          /* handle to xenctrl library (libxc) */
    uint32_t domain_id;     /* domid that we are accessing */
    char *domain_name;      /* domain name that we are accessing */
    char *sysmap;           /* system map file for domain's running kernel */
    uint32_t kpgd;          /* kernel page global directory */
    uint32_t init_task;     /* address of task struct for init */
    int os_type;            /* type of os: XA_OS_LINUX, etc */
    int hvm;                /* nonzero if HVM domain */
    xc_dominfo_t info;      /* libxc info: domid, ssidref, stats, etc */
    unsigned long *live_pfn_to_mfn_table;
    unsigned long nr_pfns;
} xa_instance_t;

/* This struct holds the task addresses that are found in a task's
   memory descriptor.  One can fill the values in the struct using
   the linux_get_taskaddr(...) function.  The comments next to each
   entry are taken from Bovet & Cesati's excellent book Understanding
   the Linux Kernel 3rd Ed, p354. */
typedef struct xa_linux_taskaddr{
    unsigned long start_code;  /* initial address of executable code */
    unsigned long end_code;    /* final address of executable code */
    unsigned long start_data;  /* initial address of initialized data */
    unsigned long end_data;    /* final address of initialized data */
    unsigned long start_brk;   /* initial address of the heap */
    unsigned long brk;         /* current final address of the heap */
    unsigned long start_stack; /* initial address of user mode stack */
    unsigned long arg_stack;   /* initial address of command-line arguments */
    unsigned long arg_end;     /* final address of command-line arguments */
    unsigned long env_start;   /* initial address of environmental variables */
    unsigned long env_end;     /* final address of environmental variables */
} xa_linux_taskaddr_t;

/*--------------------------------------------------------
 * Initialization and Destruction functions from xa_core.c
 */

/**
 * Initializes access to a specific domU given a domain id.  The
 * domain id must represent an active domain and must be > 0.  All
 * calls to xa_init must eventually call xa_destroy.
 *
 * @param[in] domain_id Domain to access
 * @param[in,out] instance Struct that holds initialization information
 * @return 0 on success, -1 on failure
 */
int xa_init (uint32_t domain_id, xa_instance_t *instance);

/**
 * Destroys an instance by freeing memory and closing any open handles.
 *
 * @param[in] instance Instance to destroy
 * @return 0 on success, -1 on failure
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
 * @param[in] instance libxa instance
 * @param[in] symbol Desired kernel symbol to access
 * @param[out] offset Offset to kernel symbol within the mapped memory
 * @return Mapped memory or NULL on error
 */
void *xa_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset);

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with a virtual address (e.g.,
 * from System.map on linux).  This memory must be unmapped manually
 * with munmap.
 *
 * @param[in] instance libxa instance
 * @param[in] virt_address Desired virtual address to access
 * @param[out] offset Offset to kernel symbol within the mapped memory
 * @return Mapped memory or NULL on error
 */
void *xa_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset);

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with a virtual address from a 
 * process' address space.  This memory must be unmapped manually
 * with munmap.
 *
 * @param[in] instance libxa instance
 * @param[in] virt_address Desired virtual address to access
 * @param[out] offset Offset to kernel symbol within the mapped memory
 * @param[in] pid (only required for user memory) PID of process that owns
 *      virtual address given in virt_address
 * @return Mapped memory or NULL on error
 */
void *xa_access_user_virtual_address (
        xa_instance_t *instance, uint32_t virt_address,
        uint32_t *offset, int pid);

/*-----------------------------
 * Linux-specific functionality
 */
int xa_linux_get_taskaddr (
        xa_instance_t *instance, int pid, xa_linux_taskaddr_t *taskaddr);


#endif /* LIB_XEN_ACCESS_H */
