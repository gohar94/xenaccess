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
 * Performs the translation from a kernel virtual address to a
 * physical address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] virt_address Desired kernel virtual address to translate
 * @return Physical address, or zero on error
 */
uint32_t xa_translate_kv2p(xa_instance_t *instance, uint32_t virt_address);

/*---------------------------------------
 * Memory access functions from xa_util.c
 */

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


/**
 * @mainpage
 *
 * The XenAccess project was inspired by ongoing research within the 
 * Georgia Tech Information Security Center (GTISC).  The purpose of this
 * library is to make it easier for other researchers to experiment with
 * the many uses of memory introspection without needing to focus on the
 * low-level details of introspection.  If you are using this library and
 * come up with a useful extension to it, we are always happy to receive
 * patches.
 *
 * Please direct all questions about XenAccess to the mailing list:
 * https://lists.sourceforge.net/lists/listinfo/xenaccess-devel
 *
 * The project was created and is maintained by Bryan D. Payne, who is
 * currently working towards his PhD in Computer Science at Georgia Tech.
 * Bryan may be reached by email at bryan@thepaynes.cc.
 *
 *
 * @section intro Introduction
 * @subsection intro1 What is memory introspection?
 * @subsection intro2 What is XenAccess?
 *
 *
 * @section install Installation
 * @subsection install1 Getting XenAccess
 * Since you are reading this document, you likely already have a copy of
 * XenAccess.  However, if you do need to download a copy, you can get the 
 * latest released version from sourceforge using the following link:
 * http://sf.net/project/platformdownload.php?group_id=159196
 *
 * You can also grab the development version directly from the subversion
 * repository.  To do this, you will need a subversion client capable of
 * handling SSL.  Then, perform the checkout with the following command:
@verbatim
 svn co https://xenaccess.svn.sf.net/svnroot/xenaccess/trunk/libxa @endverbatim
 *
 * @subsection install2 Building XenAccess
 * Before compiling XenAccess, you should make sure that you have a standard
 * development environment installed including gcc, make, autoconf, etc.
 * You will also need the libxc library, which is included with a typical
 * Xen installation.  XenAccess uses the standard GNU build system.  To
 * compile the library, follow the steps shown below.
@verbatim
./autogen.sh
./configure
make @endverbatim
 *
 * Note that you can specify options to the configure script to specify, for
 * example, the installation location.  For a complete list of configure
 * options, run:
@verbatim
./configure --help @endverbatim
 *
 * @subsection install3 Installing XenAccess
 * Installation is optional.  This is useful if you will be developing code
 * to use the XenAccess library.  However, if you are just running the examples,
 * then there is no need to do an installation.  If you choose to install
 * XenAccess, you can do it using the steps shown below:
@verbatim
su 
make install @endverbatim
 *
 * Note that this will install XenAccess under the install prefix spcified to
 * the configure script.  If you did not specify an install prefix, then
 * XenAccess is installed under /usr/local.
 *
 * @subsection install4 Configuring XenAccess
 * In order to work properly, XenAccess requires that you install a
 * configuration file.  This file has a set of entries for each domain that 
 * XenAccess will access.  These entries specify things such as the OS type
 * (e.g., Linux or Windows), the location of symbolic information, and offsets
 * used to access data within the domain.  The file format is relatively
 * straight forward.  The generic format is shown below:
@verbatim
<domain name> {
    <key> = <value>;
    <key> = <value>;
} @endverbatim
 *
 * The domain name is what appears when you use the 'xm list' command.  There
 * are 14 different keys available for use.  The ostype and sysmap
 * keys are used by both Linux and Windows domains.  The available keys are
 * @li @c ostype Linux or Windows guests are supported.
 * @li @c sysmap The path to the System.map file or the exports file (details below).
 * @li @c linux_tasks The number of bytes (offset) from the start of the struct until task_struct->tasks from linux/sched.h in the domain's kernel.
 * @li @c linux_mm Offset to task_struct->mm.
 * @li @c linux_pid Offset to task_struct->pid.
 * @li @c linux_name Offset to task_struct->name.
 * @li @c linux_pgd Offset to task_struct->pgd.
 * @li @c linux_addr Offset to task_struct->start_code.
 * @li @c win_tasks Offset to EPROCESS->ActiveProcessLinks.
 * @li @c win_pdbase Offset to EPROCESS->Pcb->DirectoryTableBase.
 * @li @c win_pid Offset to EPROCESS->UniqueProcessId.
 * @li @c win_peb Offset to EPROCESS->Peb.
 * @li @c win_iba Offset to EPROCESS->Peb->ImageBaseAddress.
 * @li @c win_ph Offset to EPROCESS->Peb->ProcessHeap.
 *
 * All of the offsets can be specified in either hex or decimal.  For hex, the
 * number should be preceeded with a '0x'.  An example configuration file is
 * shown below:
@verbatim
Fedora-HVM {
    sysmap      = "/boot/System.map-2.6.18-1.2798.fc6";
    ostype      = "Linux";
    linux_tasks = 268;
    linux_mm    = 276;
    linux_pid   = 312;
    linux_name  = 548;
    linux_pgd   = 40;
    linux_addr  = 132;
}

WinXPSP2 {
    ostype      = "Windows";
    win_tasks   = 0x88;
    win_pdbase  = 0x18;
    win_pid     = 0x84;
    win_peb     = 0x1b0;
    win_iba     = 0x8;
    win_ph      = 0x18;
} @endverbatim
 *
 * You can specify as many domains as you wish in this configuration file.
 * When you are done creating this file, it must be saved to
 * /etc/xenaccess.conf.
 *
 * @section examples Example Code
 * @subsection examples1 Included Examples
 * @subsection examples2 In Detail: process-list.c
 * @subsection examples3 Running the Examples
 *
 *
 * @section devel Programming With XenAccess
 * @subsection devel1 Best Practices
 * @subsection devel2 Patterns
 * @subsection devel3 Debugging
 * @subsection devel4 Bridging the Semantic Gap
 *
 *
 * @section problems Troubleshooting
 */

#endif /* LIB_XEN_ACCESS_H */
