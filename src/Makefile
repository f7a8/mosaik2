include ../config.mk

TARGETS = \
	  libmosaik2.c \
	  init.c \
	  index.c \
	  tiler.c \
	  gathering.c \
	  join.c \
	  duplicates.c \
	  invalid.c \
	  info.c \
	  crop.c \
	  mosaik2.c

ifeq (${phash},1)
	TARGETS += \
		duplicates_phash.c
endif

OBJECTS = ${TARGETS:.c=.o}

I_SRCS = ${shell echo *.raw}
I_DSTS = ${I_SRCS:.raw=.inc}

mosaik2: deps.mk ${OBJECTS} ${I_DSTS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJECTS} ${LDLIBS}

include deps.mk

.raw.inc:
	sed 's/^\(.*\)$$/"\1\\n"/' < ${@:.inc=.raw} > $@

help.inc: help.raw
usage.inc: usage.raw
version.inc: version.raw



mosaik2: ${OBJECTS} ${I_DSTS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJECTS} ${LDLIBS}

deps.mk: ${TARGETS} ${I_DSTS}
	${CC} ${CFLAGS} -MM ${TARGETS} > $@

clean:
	rm -f mosaik2 *.o *.inc



.PHONY: clean

.SUFFIXES: .inc .raw
