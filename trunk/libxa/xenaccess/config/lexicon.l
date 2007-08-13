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
 * Definition of tokens for the configuration file.
 *
 * File: lexicon.l
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */

#include <string.h>
#include "grammar.h"
%}

%%
sysmap                  return SYSMAPTOK;
ostype                  return OSTYPETOK;
[a-zA-Z0-9.-]+          yylval.str = strdup(yytext); return WORD;
[a-zA-Z0-9\/.-]+        yylval.str = strdup(yytext); return FILENAME;
\"                      return QUOTE;
\{                      return OBRACE;
\}                      return EBRACE;
;                       return SEMICOLON;
=                       return EQUALS;
\n                      /* ignore EOL */;
[ \t]+                  /* ignore whitespace */;
%%
