include ../config.mk

SOURCES = ${shell echo *.pre}
TARGETS = ${SOURCES:.pre=.1}

all: ${TARGETS}

.pre.1: 
	sed \
	-e 's/\$$VERSION\$$/${VERSION}/g' \
	-e 's/\$$DATE\$$/${MAN_DATE}/g' \
	-e 's/\$$MAN_DEBUG\$$/${MAN_DEBUG}/' \
	-e 's/\$$MAN_CURL\$$/${MAN_CURL}/' \
	-e 's/\$$MAN_EXIF\$$/${MAN_EXIF}/' \
	-e 's/\$$MAN_PHASH\$$/${MAN_PHASH}/' \
	-e 's/\$$MAN_AVX\$$/${MAN_AVX}/' \
	< ${@:.1=.pre} > $@

clean:
	rm -f *.1

.SUFFIXES: .pre .1

.PHONY: clean
