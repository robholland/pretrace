#
# $Id: Makefile,v 1.1 2005/07/27 07:45:37 taviso Exp $
#

VERSION=0.4
PREFIX=/usr/local
DOCDIR=$(PREFIX)/share/doc/pretrace-$(VERSION)
LIBDIR=$(PREFIX)/lib
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man

all: libpretrace.so ptgenmap doc
	@echo 'libpretrace.so has been built, please see README for instructions.'
	@echo 'type `make install` to install to $(PREFIX)'

ptgenmap: libpretrace.so ptgenmap.o
	$(CC) $(CFLAGS) $(LDFLAGS) -g -L. -lpretrace ptgenmap.o -o ptgenmap

ptgenmap.o:
	$(CC) $(CFLAGS) -Wall -g -fPIC -c ptgenmap.c

libpretrace.o:
	$(CC) $(CFLAGS) -Wall -g -fPIC -c libpretrace.c

trie.o:
	$(CC) $(CFLAGS) -Wall -g -fPIC -c trie.c

libpretrace.so: libpretrace.o trie.o
	$(CC) $(CFLAGS) $(LDFLAGS) -g -shared -o libpretrace.so libpretrace.o trie.o

doc:
	gzip -9c pretrace.3 > pretrace.3.gz
	gzip -9c ptgenmap.8 > ptgenmap.8.gz

install: all
	@echo 'installing libpretrace to $(PREFIX)...'
	install -d $(LIBDIR) $(DOCDIR)
	install -m 0755 libpretrace.so $(LIBDIR)
	install -m 0755 ptgenmap $(BINDIR)
	install -m 0644 README pretrace.conf.example ChangeLog $(DOCDIR)
	install -m 0644 pretrace.3.gz $(MANDIR)/man3
	install -m 0644 ptgenmap.8.gz $(MANDIR)/man8

clean:
	-rm -f libpretrace.so libpretrace.o trie.o ptgenmap.o ptgenmap *~
	-rm -f pretrace.3.gz ptgenmap.8.gz *.core
	@-rm -rf libpretrace-$(VERSION).tar.gz libpretrace-$(VERSION)

dist: clean
	mkdir libpretrace-$(VERSION)
	cp libpretrace.c pretrace.h ptgenmap.c trie.c trie.h README pretrace.conf.example Makefile ChangeLog ptgenmap.8 pretrace.3 libpretrace-$(VERSION)
	tar -zcvf libpretrace-$(VERSION).tar.gz libpretrace-$(VERSION)
	@-rm -rf libpretrace-$(VERSION)
	@-ls -l libpretrace-$(VERSION).tar.gz

commit: clean
	indent -kr -nut -l200 *.c *.h
	svn commit
