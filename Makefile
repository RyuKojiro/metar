CFLAGS=-lcurl
BUILDDIR=bin
BIN=metar
MAN=metar.1
SRCS=src/main.c
MANPAGE=src/$(MAN)
PROG=$(BUILDDIR)/$(BIN)
MANDEST=/usr/share/man/man1/
PREFIX?=/usr/local/bin/

$(PROG): $(BUILDDIR) $(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS)

$(BUILDDIR):
	mkdir $(BUILDDIR)

clean:
	rm -f $(OBJS) $(PROG)

install: $(PROG)
	install $(PROG) $(PREFIX)
	install $(MANPAGE) $(MANDEST)

uninstall:
	rm -f $(PREFIX)/$(BIN) $(MANDEST)/$(MAN)

.PHONY: all clean install uninstall
