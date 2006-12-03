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

dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_OUTPUT(Makefile \
          xenaccess.pc \
          xenaccess/Makefile \
          examples/Makefile
)

