%{
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
 * Definition of grammar for the configuration file.
 *
 * File: grammar.y
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "config_parser.h"

xa_config_entry_t entry;
xa_config_entry_t tmp_entry;
char *target_domain = NULL;
char tmp_str[CONFIG_STR_LENGTH];

void yyerror(const char *str)
{
    fprintf(stderr,"error: %s\n",str);
}
 
int yywrap()
{
    return 1;
}

void entry_done ()
{
    if (strncmp(tmp_entry.domain_name, target_domain, CONFIG_STR_LENGTH) == 0){
        entry = tmp_entry;
/*
        memcpy(entry.domain_name, tmp_entry.domain_name, CONFIG_STR_LENGTH);
        memcpy(entry.sysmap, tmp_entry.sysmap, CONFIG_STR_LENGTH);
        memcpy(entry.ostype, tmp_entry.ostype, CONFIG_STR_LENGTH)
        entry.offsets = tmp_entry.offsets;
*/
    }
}

xa_config_entry_t* xa_get_config()
{
    return &entry;
}
  
void xa_parse_config(char *td)
{
    target_domain = strdup(td);
    yyparse();
    if (target_domain) free(target_domain);
} 

%}

%union{
    char *str;
}

%token<str>    NUM
%token         LINUX_TASKS
%token         LINUX_MM
%token         LINUX_PID
%token         LINUX_NAME
%token         LINUX_PGD
%token         LINUX_ADDR
%token         WIN_TASKS
%token         WIN_PDBASE
%token         WIN_PID
%token         WIN_PEB
%token         WIN_IBA
%token         WIN_PH
%token         SYSMAPTOK
%token         OSTYPETOK
%token<str>    WORD
%token<str>    FILENAME
%token         QUOTE
%token         OBRACE
%token         EBRACE
%token         SEMICOLON
%token         EQUALS

%%
domains:
        |
        domains domain_info
        ;

domain_info:
        WORD OBRACE assignments EBRACE
        {
            snprintf(tmp_str, CONFIG_STR_LENGTH,"%s", $1);
            memcpy(tmp_entry.domain_name, tmp_str, CONFIG_STR_LENGTH);
            entry_done();
        }
        ;

assignments:
        |
        assignments assignment SEMICOLON
        ;

assignment:
        |
        sysmap_assignment
        |
        ostype_assignment
        |
        linux_tasks_assignment
        |
        linux_mm_assignment
        |
        linux_pid_assignment
        |
        linux_name_assignment
        |
        linux_pgd_assignment
        |
        linux_addr_assignment
        |
        win_tasks_assignment
        |
        win_pdbase_assignment
        |
        win_pid_assignment
        |
        win_peb_assignment
        |
        win_iba_assignment
        |
        win_ph_assignment
        ;

linux_tasks_assignment:
        LINUX_TASKS EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.linux_offsets.tasks = tmp;
        }
        ;

linux_mm_assignment:
        LINUX_MM EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.linux_offsets.mm = tmp;
        }
        ;

linux_pid_assignment:
        LINUX_PID EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.linux_offsets.pid = tmp;
        }
        ;

linux_name_assignment:
        LINUX_NAME EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.linux_offsets.name = tmp;
        }
        ;

linux_pgd_assignment:
        LINUX_PGD EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.linux_offsets.pgd = tmp;
        }
        ;

linux_addr_assignment:
        LINUX_ADDR EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.linux_offsets.addr = tmp;
        }
        ;

win_tasks_assignment:
        WIN_TASKS EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.windows_offsets.tasks = tmp;
        }
        ;

win_pdbase_assignment:
        WIN_PDBASE EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.windows_offsets.pdbase = tmp;
        }
        ;

win_pid_assignment:
        WIN_PID EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.windows_offsets.pid = tmp;
        }
        ;

win_peb_assignment:
        WIN_PEB EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.windows_offsets.peb = tmp;
        }
        ;

win_iba_assignment:
        WIN_IBA EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.windows_offsets.iba = tmp;
        }
        ;

win_ph_assignment:
        WIN_PH EQUALS NUM
        {
            int tmp = strtol($3, NULL, 0);
            tmp_entry.offsets.windows_offsets.ph = tmp;
        }
        ;

sysmap_assignment:
        SYSMAPTOK EQUALS QUOTE FILENAME QUOTE 
        {
            snprintf(tmp_str, CONFIG_STR_LENGTH,"%s", $4);
            memcpy(tmp_entry.sysmap, tmp_str, CONFIG_STR_LENGTH);
        }
        ;

ostype_assignment:
        OSTYPETOK EQUALS QUOTE WORD QUOTE 
        {
            snprintf(tmp_str, CONFIG_STR_LENGTH,"%s", $4);
            memcpy(tmp_entry.ostype, tmp_str, CONFIG_STR_LENGTH);
        }
        ;
%%
