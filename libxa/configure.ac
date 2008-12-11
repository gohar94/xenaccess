dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_INIT(xenaccess/xenaccess.h)
AM_CONFIG_HEADER(config.h)

dnl -----------------------------------------------
dnl Package name and version number (user defined)
dnl -----------------------------------------------

LIBRARY_NAME=xenaccess

#release versioning
MAJOR_VERSION=0
MINOR_VERSION=5
MICRO_VERSION=0

dnl --------------------------------
dnl Package name and version number
dnl --------------------------------

PACKAGE=$LIBRARY_NAME
AC_SUBST(LIBRARY_NAME)

VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION
RELEASE=$MAJOR_VERSION.$MINOR_VERSION
AC_SUBST(RELEASE)
AC_SUBST(VERSION)

VERSION=$VERSION

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)

dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------

AC_PROG_CC
AM_PROG_LIBTOOL
AM_SANITY_CHECK

AC_CHECK_LIB(xenstore, xs_read, [LIBS="-lxenstore $LIBS"], [exit -1])
AC_CHECK_LIB(xenctrl, xc_interface_open, [LIBS="-lxenctrl $LIBS"], [exit -1])

AC_CHECK_PROGS(YACC,yacc bison byacc,[no],[path = $PATH])
[if    test "$YACC" = "no"]
[then]
   [echo "yacc not found in the search path. Please ensure that it is"]
   [echo "installed and its directory is included in the search path".]
   [exit 1]
[else]
   [echo "Found yacc as $YACC."]
[fi]
AC_PROG_YACC

AC_CHECK_PROGS(LEX,lex flex ,[no],[path = $PATH])
[if    test "$LEX" = "no"]
[then]
   [echo "lex not found in the search path. Please ensure that it is"]
   [echo "installed and its directory is included in the search path".]
   [exit 1]
[else]
   [echo "Found lex as $LEX."]
[fi]
AC_PROG_LEX

AC_CHECK_FUNCS(xc_map_foreign_pages, AC_DEFINE([HAVE_MAP_FOREIGN], [], [Indicates existance of xc_map_foreign_pages]))

dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_OUTPUT(Makefile \
          xenaccess.pc \
          xenaccess/Makefile \
          xenaccess/config/Makefile \
          examples/Makefile
)

