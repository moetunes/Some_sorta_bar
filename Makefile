CFLAGS+= -Wall
LDADD+= -lX11
LDFLAGS=
EXEC=some_sorta_bar

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

some_sorta_bar: some_sorta_bar.o
	$(CC) $(LDFLAGS) -s -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 some_sorta_bar $(DESTDIR)$(BINDIR)/some_sorta_bar

clean:
	rm -fv some_sorta_bar *.o

