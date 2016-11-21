CFLAGS=-lcurl
SRCS=src/main.c
BUILDDIR=bin

PROG=metar
MANPAGE=metar.1

MAN=src/$(MANPAGE)
BIN=$(BUILDDIR)/$(PROG)
PREFIX?=/usr/local
MANDEST=$(PREFIX)/share/man/man1
BINDEST=$(PREFIX)/bin

$(BIN): $(BUILDDIR) $(SRCS)
	$(CC) $(CFLAGS) -o $(BIN) $(SRCS)

$(BUILDDIR):
	mkdir $(BUILDDIR)

clean:
	rm -f $(OBJS) $(BIN)

install: $(BIN)
	install $(BIN) $(BINDEST)
	install $(MAN) $(MANDEST)

uninstall:
	rm -f $(BINDEST)/$(PROG) $(MANDEST)/$(MANPAGE)

.PHONY: all clean install uninstall
