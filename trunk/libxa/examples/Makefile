CC       = gcc

CFLAGS   += -Wall
CFLAGS   += -Werror
CFLAGS   += -O3
CFLAGS   += $(INCLUDES) -I.
CFLAGS   += -Wp,-MD,.$(@F).d
CFLAGS   += -m32 -march=i686
LDFLAGS  += -L. -L../
LDFLAGS  += -m32
DEPS     = .*.d

all: module-list process-list

clean:
	rm -rf *.a *.o *~ $(DEPS) module-list process-list

module-list: module-list.c
	$(CC) $(CFLAGS) $(LDFLAGS) -l xenctrl -l xenaccess -o $@ $^

process-list: process-list.c
	$(CC) $(CFLAGS) $(LDFLAGS) -l xenctrl -l xenaccess -o $@ $^

-include $(DEPS)
