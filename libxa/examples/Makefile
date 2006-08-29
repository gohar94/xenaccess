CC       = gcc

CFLAGS   += -Wall
CFLAGS   += -Werror
CFLAGS   += -O3
CFLAGS   += $(INCLUDES) -I.
CFLAGS   += -Wp,-MD,.$(@F).d
LDFLAGS  += -L. -L../
DEPS     = .*.d

all: module-list process-list process-data

clean:
	rm -rf *.a *.o *~ $(DEPS) module-list process-list process-data

module-list: module-list.c
	$(CC) $(CFLAGS) $(LDFLAGS) -l xenctrl -l xenaccess -o $@ $^

process-list: process-list.c
	$(CC) $(CFLAGS) $(LDFLAGS) -l xenctrl -l xenaccess -o $@ $^

process-data: process-data.c
	$(CC) $(CFLAGS) $(LDFLAGS) -l xenctrl -l xenaccess -o $@ $^

-include $(DEPS)
