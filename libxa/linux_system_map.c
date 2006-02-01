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
 * This file contains utility functions reading information from the
 * System.map file which contains symbol information from the linux
 * kernel created by nm.
 *
 * File: linux_system_map.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "xa_private.h"

/* 80 chars is more than enough for one line of system.map */
#define MAX_ROW_LENGTH 200

int get_system_map_row (FILE *f, char *row, char *symbol, int position)
{
    int ret = XA_FAILURE;

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

int linux_system_map_symbol_to_address (
        xa_instance_t *instance, char *symbol, uint32_t *address)
{
    /* hard code this for now, but we need to figure out how
     * to automate the discovery of this location */
    char *system_map = "/boot/System.map-2.6.12.6-xen";

    FILE *f = NULL;
    char *row = NULL;
    int ret = XA_SUCCESS;

    if ((row = malloc(MAX_ROW_LENGTH)) == NULL ){
        ret = XA_FAILURE;
        goto error_exit;
    }
    if ((f = fopen(system_map, "r")) == NULL){
        ret = XA_FAILURE;
        goto error_exit;
    }
    if (get_system_map_row(f, row, symbol, 2) == XA_FAILURE){
        ret = XA_FAILURE;
        goto error_exit;
    }

    *address = (uint32_t) strtoul(row, NULL, 16);

error_exit:
    if (row) free(row);
    if (f) fclose(f);
    return ret;
}
