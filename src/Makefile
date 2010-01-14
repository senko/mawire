PKGS = hildon-1 hildon-fm-2 sqlite3 dbus-glib-1 gconf-2.0

CC = gcc
CFLAGS = -Wall -Werror -g -O3 $$(pkg-config --cflags $(PKGS))
LDFLAGS = -g -O3 $$(pkg-config --libs $(PKGS))
OBJS = app.o util.o db.o ui.o

.PHONY: all clean

all: mawire

clean:
	rm -f mawire *.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

mawire: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

install: mawire
	install -d ${DESTDIR}/opt/mawire/lib
	install mawire ${DESTDIR}/opt/mawire/lib

