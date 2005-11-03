CC       = gcc

SRCS       :=
SRCS       += xa_core.c
SRCS       += xa_memory.c
SRCS       += xa_pretty_print.c

CFLAGS   += -Wall
CFLAGS   += -Werror
CFLAGS   += -O3
CFLAGS   += $(INCLUDES) -I.
CFLAGS   += -Wp,-MD,.$(@F).d
CFLAGS   += -m32 -march=i686
LDFLAGS  += -L.
LDFLAGS  += -m32
DEPS     = .*.d

LIB_OBJS := $(patsubst %.c,%.o,$(SRCS))
PIC_OBJS := $(patsubst %.c,%.opic,$(SRCS))

LIB := libxenaccess.a
LIB += libxenaccess.so 

all: build
build: 
	$(MAKE) $(LIB)

clean:
	rm -rf *.a *.so* *.o *.opic *.rpm $(LIB) *~ $(DEPS) test

# libxenaccess
libxenaccess.a: $(LIB_OBJS)
	$(AR) rc $@ $^

libxenaccess.so: $(PIC_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-soname -Wl,libxenaccess.so -shared -o $@ $^

#general build rules
%.opic: %.c
	$(CC) $(CPPFLAGS) -DPIC $(CFLAGS) -fPIC -c -o $@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.cc
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# test program
test: test.c
	$(CC) $(CFLAGS) $(LDFLAGS) -l xenctrl -l xenaccess -o $@ $^

-include $(DEPS)
