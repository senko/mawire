PKGS = hildon-1 hildon-fm-2 sqlite3 dbus-glib-1 gconf-2.0

CC = gcc
CFLAGS = -Wall -Werror -g -O3 $$(pkg-config --cflags $(PKGS))
LDFLAGS = -g -O3 $$(pkg-config --libs $(PKGS))
OBJS = app.o util.o db.o ui.o

.PHONY: all clean

all: maemopaedia

clean:
	rm -f maemopaedia *.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

maemopaedia: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

install: maemopaedia
	install -d ${DESTDIR}/opt/maemopaedia/lib
	install maemopaedia ${DESTDIR}/opt/maemopaedia/lib

