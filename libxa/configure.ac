
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
MINOR_VERSION=3
MICRO_VERSION=0

#API version (often = MAJOR_VERSION.MINOR_VERSION)
API_VERSION=0.3
AC_SUBST(API_VERSION)

#shared library versioning
LIBRARY_VERSION=0:3:0
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                                  or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed


dnl --------------------------------
dnl Package name and version number
dnl --------------------------------

AC_SUBST(LIBRARY_VERSION)

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

dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_OUTPUT(Makefile \
          xenaccess.pc \
          xenaccess/Makefile
)

