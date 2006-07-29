CC       = gcc

SRCS       :=
SRCS       += xa_core.c
SRCS       += xa_memory.c
SRCS       += xa_cache.c
SRCS       += xa_pretty_print.c
SRCS       += linux_system_map.c
SRCS       += linux_memory.c
SRCS       += linux_domain_info.c

CFLAGS   += -Wall
CFLAGS   += -Werror
CFLAGS   += -ansi
#CFLAGS   += -pedantic
CFLAGS   += -O3 -g
CFLAGS   += $(INCLUDES) -I.
CFLAGS   += -Wp,-MD,.$(@F).d
#CFLAGS   += -m32 -march=i686
CFLAGS   += $(shell xml2-config --cflags)
LDFLAGS  += -L.
LDFLAGS  += -m32
LDFLAGS  += -lm
LDFLAGS  += -lvirt
LDFLAGS  += $(shell xml2-config --libs)
DEPS     = .*.d
STD      = -std=c99

LIB_OBJS := $(patsubst %.c,%.o,$(SRCS))
PIC_OBJS := $(patsubst %.c,%.opic,$(SRCS))

LIB := libxenaccess.a
LIB += libxenaccess.so 

all: build
build: 
	$(MAKE) $(LIB)

clean:
	rm -rf *.a *.so* *.o *.opic *.rpm $(LIB) *~ $(DEPS) 

# libxenaccess
libxenaccess.a: $(LIB_OBJS)
	$(AR) rc $@ $^

libxenaccess.so: $(PIC_OBJS)
	$(CC) $(STD) $(CFLAGS) $(LDFLAGS) -Wl,-soname -Wl,libxenaccess.so -shared -o $@ $^

#general build rules
%.opic: %.c
	$(CC) $(STD) $(CPPFLAGS) -DPIC $(CFLAGS) -fPIC -c -o $@ $<

%.o: %.c
	$(CC) $(STD) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.cc
	$(CC) $(STD) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

-include $(DEPS)
