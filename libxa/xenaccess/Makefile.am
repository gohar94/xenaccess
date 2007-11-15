SUBDIRS = config

h_sources = xenaccess.h xa_private.h
c_sources = linux_core.c linux_domain_info.c linux_system_map.c xa_core.c xa_memory.c linux_memory.c xa_cache.c xa_domain_info.c xa_pretty_print.c xa_util.c windows_memory.c windows_symbols.c windows_core.c

library_includedir=$(includedir)/$(LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES= libxenaccess.la
libxenaccess_la_SOURCES= $(h_sources) $(c_sources)
libxenaccess_la_LIBADD= config/libconfig.la
libxenaccess_la_LDFLAGS= -release $(RELEASE)
