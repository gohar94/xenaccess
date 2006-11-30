## Source directory

SUBDIRS = 

h_sources = xenaccess.h xa_private.h
c_sources = linux_domain_info.c linux_system_map.c xa_core.c xa_memory.c linux_memory.c xa_cache.c xa_domain_info.c xa_pretty_print.c

library_includedir=$(includedir)/$(LIBRARY_NAME)-$(API_VERSION)/$(LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES= libxenaccess-0.3.la
libxenaccess_0_3_la_SOURCES= $(h_sources) $(c_sources)
libxenaccess_0_3_la_LDFLAGS= -version-info $(LIBRARY_VERSION) -release $(RELEASE)

