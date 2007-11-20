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
/**
 * Constant used to indicate that we are running on a version of Xen
 * that XenAccess does not support.  XenAccess might work, or it might
 * not.  This is used in the xen_version member of the xa_instance struct.
 */
#define XA_XENVER_UNKNOWN 0
/**
 * Constant used to indicate that we are running on Xen 3.0.4-1.  This
 * is used in the xen_version member of the xa_instance struct.
 */
#define XA_XENVER_3_0_4 1
/**
 * Constant used to indicate that we are running on Xen 3.0.1  This 
 * is used in the xen_version member of the xa_instance struct.
 */
#define XA_XENVER_3_1_0 2

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
    int xen_version;        /**< version of Xen libxa is running on */
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
 * @brief Linux task information.
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
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_init (uint32_t domain_id, xa_instance_t *instance);

/**
 * Destroys an instance by freeing memory and closing any open handles.
 *
 * @param[in] instance Instance to destroy
 * @return XA_SUCCESS or XA_FAILURE
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
 * @param[out] value The value read from memory
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_read_long_sym (xa_instance_t *instance, char *sym, uint32_t *value);

/**
 * Reads a long long (64 bit) value from memory, given a kernel symbol.
 *
 * @param[in] instance XenAccess instance
 * @param[in] sym Kernel symbol to read from
 * @param[out] value The value read from memory
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_read_long_long_sym (xa_instance_t *instance, char *sym, uint64_t *value);

/**
 * Reads a long (32 bit) value from memory, given a virtual address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] vaddr Virtual address to read from
 * @param[in] pid Pid of the virtual address space (0 for kernel)
 * @param[out] value The value read from memory
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_read_long_virt (
        xa_instance_t *instance, uint32_t vaddr, int pid, uint32_t *value);

/**
 * Reads a long long (64 bit) value from memory, given a virtual address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] vaddr Virtual address to read from
 * @param[in] pid Pid of the virtual address space (0 for kernel)
 * @param[out] value The value read from memory
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_read_long_long_virt (
        xa_instance_t *instance, uint32_t vaddr, int pid, uint64_t *value);

/**
 * Reads a long (32 bit) value from memory, given a physical address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] paddr Physical address to read from
 * @param[out] value The value read from memory
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_read_long_phys (
        xa_instance_t *instance, uint32_t paddr, uint32_t *value);

/**
 * Reads a long long (64 bit) value from memory, given a physical address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] paddr Physical address to read from
 * @param[out] value The value read from memory
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_read_long_long_phys (
        xa_instance_t *instance, uint32_t paddr, uint64_t *value);

/**
 * Reads a long (32 bit) value from memory, given a machine address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] maddr Machine address to read from
 * @param[out] value The value read from memory
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_read_long_mach (
        xa_instance_t *instance, uint32_t maddr, uint32_t *value);

/**
 * Reads a long long (64 bit) value from memory, given a machine address.
 *
 * @param[in] instance XenAccess instance
 * @param[in] maddr Machine address to read from
 * @param[out] value The value read from memory
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_read_long_long_mach (
        xa_instance_t *instance, uint32_t maddr, uint64_t *value);

/*-----------------------------
 * Linux-specific functionality
 */

/**
 * Extracts information about the specified process' location in memory from
 * the task struct specified by @a pid.
 *
 * @param[in] instance XenAccess instance
 * @param[in] pid The PID for the task to read from
 * @param[out] taskaddr Information from the specified task struct
 * @return XA_SUCCESS or XA_FAILURE
 */
int xa_linux_get_taskaddr (
        xa_instance_t *instance, int pid, xa_linux_taskaddr_t *taskaddr);

/*-----------------------------
 * Windows-specific functionality
 */

/**
 * Extracts information from the PEB struct, which is located at the top of
 * the EPROCESS struct with the specified @a pid.
 *
 * @param[in] instance XenAccess instance
 * @param[in] pid The unique ID for the PEB to read from
 * @param[out] peb Information from the specified PEB
 * @return XA_SUCCESS or XA_FAILURE
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
 * https://lists.sf.net/lists/listinfo/xenaccess-devel
 *
 * The project was created and is maintained by Bryan D. Payne, who is
 * currently working towards his PhD in Computer Science at Georgia Tech.
 * Bryan may be reached by email at bryan@thepaynes.cc.
 *
 *
 * @section intro Introduction
 * @subsection intro1 What is XenAccess?
 * XenAccess is a library that simplifies the process of memory introspection
 * for virtual machines running on the Xen hypervisor.  With XenAccess, your
 * software can run in one virtual machine and access the memory space of
 * other virtual machines.  While the Xen Control Library (libxc), which is
 * included with Xen, provides the ability to access another virtual 
 * machine's memory at a low level, XenAccess allows you to access memory
 * using kernel symbols, virtual addresses, and physical addresses.
 *
 * @subsection intro2 What is memory introspection?
 * Memory introspection is the process of viewing the memory of one virtual
 * machine from a different virtual machine.  On the surface, this sounds
 * rather simple.  In fact, Xen provides a function to facilitate this type
 * of memory access.  What makes memory introspection difficult is the
 * semantic gap between the two virtual machines.  For example, to lookup
 * virtual addresses XenAccess must walk the page tables inside the other
 * virtual machine.  However, to walk these page tables, XenAccess must 
 * first know where the page directory is located (i.e., the CR3 value).
 * And this value depends on the process address space you are viewing.
 * The more you think about the problem, the reasons for its difficulty
 * become clear.  One must know a lots of details about the target operating
 * system in order to build these higher levels of abstraction.
 *
 * @image html intro-detail.png XenAccess must take several steps to access a memory page based on a kernel symbol in Linux.
 * The figure above shows the steps that XenAccess takes to access a page of
 * memory using a kernel symbol.  This figure is focused on Linux, however the
 * procedure for Windows is similar.  Instead of using the System.map file,
 * kernel symbols are converted to virtual addresses using the export values
 * from ntoskrnl.exe.
 *
 * Memory introspection is useful because it allows you to monitor and control
 * an operating system from a protected location.  Previous research has 
 * shown that introspection can be used for a wide variety of security
 * applications, but more ideas are coming out all the time.  Using XenAccess,
 * you can quickly experiment with your new ideas and help advance this
 * new an exciting research direction.
 *
 *
 * @section install Installation
 * @subsection install1 Getting XenAccess
 * You can get the latest released version of XenAccess from SourceForge
 * using the following link:
 * http://sf.net/project/platformdownload.php?group_id=159196
 *
 * You can also grab the development version directly from the subversion
 * repository.  To do this, you will need a subversion client capable of
 * handling SSL.  Then, perform the checkout with the following command:
@verbatim
 svn co https://xenaccess.svn.sf.net/svnroot/xenaccess/trunk/libxa @endverbatim
 *
 * If you are just getting started with XenAccess, you probably want to 
 * use the latest released version.  However, if you need a new feature that
 * hasn't been released, or you are planning on submitting a patch, then you
 * may want to try the development version.
 *
 * @subsection install2 Building XenAccess
 * Before compiling XenAccess, you should make sure that you have a standard
 * development environment installed including gcc, make, autoconf, etc.
 * You will also need the libxc library and the libxenstore library, which are
 * included with a typical Xen installation.  XenAccess uses the standard GNU
 * build system.  To compile the library, follow the steps shown below.
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
 * listed below:
 *
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
 * XenAccess comes with a variety of small examples to demonstrate how to use
 * the library.  These are also useful for checking that you have successfully
 * completed the build and configuration steps described in the section above.
 * The first argument for each example is the domain ID that you wish to view.
 * This should be the same ID seen using the 'xm list' command.  The domain
 * you specify must also be included in the configuration file.  The provided
 * examples are listed below:
 *
 * @li @c map-addr Dumps a memory page to stdout based on the provided virtual address.  The virtual address must be a kernel virtual address.  The page is displayed in a readable format complete with hex, ascii, and offsets.  The number printed before the memory page is the offset of the specified address within the page.
 * @li @c map-symbol Same as @c map-addr except you specify a kernel symbol instead of a kernel virtual address.
 * @li @c module-list Lists the kernel modules installed in the operating system.  This is the same list that you would get using 'lsmod' on a Linux system.  On Windows, it lists the drivers loaded into the kernel.
 * @li @c process-list Lists the running processes in the operating system.  This is the same list that you would get using 'ps -ef' on a Linux system.  On Windows, it is the same list that you would get using the task manager.
 * @li @c process-data Displays the first memory page of executable content for a given process.  The process number, which is provided as a second argument, is the number obtained from using the @c process-list example above.
 *
 * @subsection examples2 In Detail: process-list.c
 * In order to better understand how the examples work, let's take a look at
 * one of the more interesting examples.  The @c process-list example displays
 * the processes running in an operating system by walking down the linked
 * list data structure containing process information.  For each process, the
 * name and ID are extracted and printed to stdout.  To see how this is done,
 * we will step through the code one piece at a time.
 *
@verbatim
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <xenaccess/xenaccess.h>
#include <xenaccess/xa_private.h> @endverbatim
 *
 * The include list is not too surprising.  Note that xa_private.h is included
 * here, but this is only to access the function that prints a memory page to
 * stdout.  Most people will not need to include xa_private.h
 *
@verbatim
#define TASKS_OFFSET 24 * 4
#define PID_OFFSET 39 * 4 
#define NAME_OFFSET 108 * 4
#define ActiveProcessLinks_OFFSET 0x88
#define UniqueProcessId_OFFSET 0x84
#define ImageFileName_OFFSET 0x174 @endverbatim
 *
 * These offset values are important as they will allow us to find the
 * necessary data within the data structures we traverse.  The first three
 * offsets are used for Linux systems and obtained by looking at the definition
 * of task_struct.  The second three offsets are used for Windows systems and
 * obtained using windbg to view the EPROCESS struct.
 *
@verbatim
    uint32_t dom = atoi(argv[1]);

    if (xa_init(dom, &xai) == XA_FAILURE){
        perror("failed to init XenAccess library");
        goto error_exit;
    } @endverbatim
 *
 * Next we read in the domain ID to look at.  (Yes, there is no error
 * checking here so the program will seg fault if you fail to specify a
 * domain ID as an argument.)  Then we make our first call to XenAccess.
 * This call initializes the instance data structure.  Note that we only 
 * perform this initialization step once as it is a costly function call.
 *
 * From this point forward, all of the error checking code will be omited
 * from the sake of clarity.  In addition, we will only focus on the Linux
 * version of the code.  The Windows version operates in a similar fashion.
 * To see the complete version of the code, look in the examples directory
 * of your copy of XenAccess.
 *
@verbatim
    memory = xa_access_kernel_symbol(&xai, "init_task", &offset);
    memcpy(&next_process, memory + offset + TASKS_OFFSET, 4);
    list_head = next_process;
    munmap(memory, xai.page_size); @endverbatim
 *
 * The kernel symbol 'init_task' points to the beginning of the process list
 * in the Linux kernel.  So we map this memory location and then copy the 
 * pointer to the next process into both the list_head and next_process
 * variables.  The task list is a circular linked list, so we will use 
 * list_head to know when we have visited every process.  The next step here 
 * is to unmap the memory, since we are done with this page.  This is 
 * important to remember since you can only map a limited number of pages at
 * a time.  Just as you would free memory after a malloc, you should unmap
 * these pages after you are done using them.
 *
@verbatim
    while (1){
        memory = xa_access_virtual_address(&xai, next_process, &offset);
        memcpy(&next_process, memory + offset, 4);

        if (list_head == next_process){
            break;
        }

        name = (char *) (memory + offset + NAME_OFFSET - TASKS_OFFSET);
        memcpy(&pid, memory + offset + PID_OFFSET - TASKS_OFFSET, 4);
        printf("[%5d] %s\n", pid, name);
        munmap(memory, xai.page_size);
    } @endverbatim
 *
 * This loop is the bulk of the program.  We map the memory page associated
 * with the next process.  For this process we check to see if it is the
 * init_task process.  If so then we are done.  If not, then we print out
 * the process information.  Note that we are obtaining this information
 * directly from the task_struct in memory of the running Linux system that
 * we are looking at.  When we are done with this memory page, we unmap it
 * and repeat the loop.
 *
@verbatim
    if (memory) munmap(memory, xai.page_size);
    xa_destroy(&xai); @endverbatim
 *
 * The final step is cleanup.  We perform a sanity check to make sure that 
 * there mapped memory pages.  And then we call xa_destroy to free any memory
 * associated with the XenAccess instance.
 *
 * @subsection examples3 Running the Examples
 * A quick way to see XenAccess in action is to try out the example code.  You
 * should be running Xen, and running the example code as root in domain 0.
 * You should have at least one user domain running and the configuration file
 * setup for this domain.  Note the domain ID using the 'xm list' command:
 *
@verbatim
[root@bluemoon libxa]# xm list
Name                                      ID Mem(MiB) VCPUs State   Time(s)
Domain-0                                   0     1229     2 r----- 137356.4
Fedora-HVM                                 4      384     1 -b----   2292.6
fc5                                        5      384     1 -b----     15.4
[root@bluemoon libxa]# @endverbatim
 *
 * Then you can run the examples as follows:
 *
@verbatim
[root@bluemoon libxa]# cd examples/
[root@bluemoon examples]# ./module-list 5
ipv6
binfmt_misc
lp
parport_pc
parport
nvram
usbcore
[root@bluemoon examples]# ./module-list 4
autofs4
hidp
rfcomm
l2cap
bluetooth
sunrpc
ipv6
parport_pc
lp
parport
floppy
8139cp
8139too
mii
pcspkr
serio_raw
dm_snapshot
dm_zero
dm_mirror
dm_mod
ext3
jbd
[root@bluemoon examples]# @endverbatim
 *
 * Note that the example code works for both para-virtualized (i.e., PV) and
 * fully-virtualized (i.e., HVM) domains.  However, the example code uses the
 * offsets you provide in the configuration file, and some hard coded offsets
 * in the example code, to locate information in the running kernels.  For
 * this reason you may find that it fails on some kernels.
 *
@verbatim
[root@bluemoon examples]# ./process-list 5
[    1] init
[    2] migration/0
[    3] ksoftirqd/0
[    4] watchdog/0
[    5] events/0
[    6] khelper
[    7] kthread
[    8] xenwatch
[    9] xenbus
[   15] kblockd/0
[   57] pdflush
[   58] pdflush
[   60] aio/0
[   59] kswapd0
[  578] kseriod
[  685] kpsmoused
[  710] khubd
[  978] dhclient
[ 1006] syslogd
[ 1009] klogd
[ 1021] sshd
[ 1027] mingetty
[root@bluemoon examples]# ./process-list 4
[1408237823] ?
[14941936] ?S?
[    0]
[    0]
ERROR: address not in page table
failed to map memory for process list pointer: Success
[root@bluemoon examples]# @endverbatim
 *
 * Here we see that @c process-list works on one of the domains, but not the
 * other.  This is because the examples were written with a specific kernel 
 * version in mind.  Your code will need to be built to work with the specific
 * system that you plan on viewing.  Future versions of XenAccess will provide
 * tools to help simplify the process of finding the right offset values.
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
