ifndef PREFIX
PREFIX=/usr/local
endif

all:
	$(MAKE) -C src all

clean:
	$(MAKE) -C src clean

install: all
	install -D -b bin/scchclean $(PREFIX)/bin/scchclean
	install -D -s -b src/sccache $(PREFIX)/bin/sccache
	install -D -s -b src/scchadm $(PREFIX)/bin/scchadm
	install -d $(PREFIX)/man/man1
	install -b doc/*.1 $(PREFIX)/man/man1
