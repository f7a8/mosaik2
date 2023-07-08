PACKAGE ?= mosaik2
VERSION ?= ${shell git describe --dirty --tags}

app ?= 0
curl ?= 1
phash ?= 1
debug ?= 0
exif ?= 1

# Prefix for all installed files
PREFIX ?= /usr/local

# icons in /usr/share/local/icons (and other prefixes != /usr) are not
# generally supported. So ignore PREFIX and always install icons into
# /usr/share/icons if the user wants to install feh on their local machine.

# Directories for manuals, executables, docs, data, etc.
main_dir = ${DESTDIR}${PREFIX}
man_dir = ${main_dir}/share/man
bin_dir = ${main_dir}/bin
doc_dir = ${main_dir}/share/doc/feh
example_dir = ${main_dir}/share/doc/feh/examples

# default CFLAGS
CFLAGS ?= -O3 -march=native -mtune=native
CFLAGS += -Wall

ifeq (${debug},1)
	CFLAGS += -DDEBUG -O0 -g
	MAN_DEBUG = This is a debug build.
else
	MAN_DEBUG = .
endif

ifeq (${curl},1)
	CFLAGS += -DHAVE_LIBCURL
	LDLIBS += -lcurl
	MAN_CURL = enabled
else
	MAN_CURL = disabled
endif

ifeq (${exif},1)
	CFLAGS += -DHAVE_LIBEXIF
	LDLIBS += -lexif
	MAN_EXIF = enabled
else
	MAN_EXIF = disabled
endif

ifeq (${phash},1)
	CFLAGS += -DHAVE_PHASH
	LDLIBS += -lpHash -LpHash-0.9.6/src/.libs
	MAN_PHASH = enabled
else
	MAN_PHASH = disabled
endif

MAN_DATE ?= ${shell date '+%B %d, %Y'}


CFLAGS += -DPREFIX=\"${PREFIX}\" \
	-DPACKAGE=\"${PACKAGE}\" -DVERSION=\"${VERSION}\"

LDLIBS += -lm -lgd -lcrypto 
