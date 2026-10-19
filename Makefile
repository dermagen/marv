# Makefile for MARV (Unix-like systems)

CC      ?= cc
CFLAGS  ?= -O2
LDFLAGS ?=
LDLIBS  ?= -lm

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/marv

.PHONY: all test check install uninstall clean

all: marv

marv: marv.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ marv.c $(LDLIBS)

test check: marv
	bash test/run-tests.sh

install: marv
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 755 marv "$(DESTDIR)$(BINDIR)/marv"
	install -d "$(DESTDIR)$(DATADIR)/lib"
	install -m 644 lib/*.mvh lib/*.c "$(DESTDIR)$(DATADIR)/lib/"
	install -d "$(DESTDIR)$(DATADIR)/target"
	install -m 644 target/*.mvt "$(DESTDIR)$(DATADIR)/target/"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/marv"
	rm -rf "$(DESTDIR)$(DATADIR)"

clean:
	rm -f marv examples/*.c examples/*.wo examples/*.s \
	      examples/multimod/*.c examples/multimod/*.wo examples/multimod/*.s
