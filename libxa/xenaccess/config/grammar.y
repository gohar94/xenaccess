%{
/*
 * The libxa library provides access to resources in domU machines.
 *
 * Copyright (C) 2005 - 2006  Bryan D. Payne (bryan@thepaynes.cc)
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
        memcpy(entry.domain_name, tmp_entry.domain_name, CONFIG_STR_LENGTH);
        memcpy(entry.sysmap, tmp_entry.sysmap, CONFIG_STR_LENGTH);
        memcpy(entry.ostype, tmp_entry.ostype, CONFIG_STR_LENGTH);
        /* copy over other values here as they are added */
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
