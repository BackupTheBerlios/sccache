CC:=gcc
ifdef release
CFLAGS+=-O2 -DNDEBUG
else
CFLAGS+=-g -Wall
endif

.PHONY: clean

OBJS=$(CSRC:.c=.o)

SCCACHE_SRC:=fp.c md4c.c sccache.c cachestat.c
SCCHADM_SRC:=scchadm.c cachestat.c

CSRC:=$(SCCACHE_SRC)

all: sccache scchadm

sccache: $(SCCACHE_SRC:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^

scchadm: $(SCCHADM_SRC:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^

%.o %.d: %.c
	$(CC) $(CFLAGS) -c -MMD -o $*.o $<
#	./sccache d.ccache $< $*.o "$(CC) -E -MD" "$(CC) $(CFLAGS) -c"

include $(patsubst %.c,%.d,$(CSRC))

clean:
	rm -f *.o *.d
