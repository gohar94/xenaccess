## Source directory

SUBDIRS = 

INCLUDES = -I$(top_srcdir)
AM_LDFLAGS = -L$(top_srcdir)/xenaccess/.libs/
LDADD = -lxenaccess -lxenctrl -lxenstore -lm

bin_PROGRAMS = module-list process-data process-list
module_list_SOURCES = module-list.c
process_data_SOURCES = process-data.c
process_list_SOURCES = process-list.c

