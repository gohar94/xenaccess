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
 * This file contains utility functions reading information from an
 * exports file created by running the dumpbin utility.  The exports
 * file used should be from ntoskrnl, which can be obtained with
 * the following command:
 *     dumpbin /exports c:\windows\system32\ntoskrnl.exe
 *
 * File: windows_symbols.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "xa_private.h"

/* 200 chars is more than enough for one line of the exports file */
#define MAX_ROW_LENGTH 200

int get_exports_row (FILE *f, char *row, char *symbol, int position)
{
    int ret = XA_FAILURE;
    int counter = 0;

    /* move past the header */
    while (fgets(row, MAX_ROW_LENGTH, f) != NULL){
        if (++counter > 19){
            break;
        }
    }

    while (fgets(row, MAX_ROW_LENGTH, f) != NULL){
        char *token = NULL;

        /* find the correct token to check */
        int curpos = 0;
        int position_copy = position;
        while (position_copy > 0 && curpos < MAX_ROW_LENGTH){
            if (isspace(row[curpos])){
                while (isspace(row[curpos])){
                    row[curpos] = '\0';
                    ++curpos;
                }
                --position_copy;
                continue;
            }
            ++curpos;
        }
        if (position_copy == 0){
            token = row + curpos;
            while (curpos < MAX_ROW_LENGTH){
                if (isspace(row[curpos])){
                    row[curpos] = '\0';
                    break;
                }
                ++curpos;
            }
        }
        else{ /* some went wrong in the loop above */
            goto error_exit;
        }

        /* check the token */
        if (strncmp(token, symbol, MAX_ROW_LENGTH) == 0){
            ret = XA_SUCCESS;
            break;
        }
    }

error_exit:
    if (ret == XA_FAILURE){
        memset(row, 0, MAX_ROW_LENGTH);
    }
    return ret;
}

uint32_t get_rva (char *row)
{
    char *p = row+17;
    uint32_t rva = (uint32_t) strtoul(p, NULL, 16);
    return rva;
}

int windows_symbol_to_address (
        xa_instance_t *instance, char *symbol, uint32_t *address)
{
    FILE *f = NULL;
    char *row = NULL;
    int ret = XA_SUCCESS;

    if ((NULL == instance->sysmap) || (strlen(instance->sysmap) == 0)){
        printf("ERROR: windows sysmap file not specified in config file\n");
        ret = XA_FAILURE;
        goto error_exit;
    }

    if ((row = malloc(MAX_ROW_LENGTH)) == NULL ){
        ret = XA_FAILURE;
        goto error_exit;
    }
    if ((f = fopen(instance->sysmap, "r")) == NULL){
        printf("ERROR: could not find exports file after checking:\n");
        printf("\t%s\n", instance->sysmap);
        printf("To fix this problem, add the correct sysmap entry to /etc/xenaccess.conf\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    if (get_exports_row(f, row, symbol, 4) == XA_FAILURE){
        ret = XA_FAILURE;
        goto error_exit;
    }

    *address = get_rva(row);

error_exit:
    if (row) free(row);
    if (f) fclose(f);
    return ret;
}
