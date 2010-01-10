
all:
	(cd sqlite-3.6.22-fts3 && \
		./configure --libdir=/usr/lib/maemopaedia && \
		$(MAKE))

install:
	(cd sqlite-3.6.22-fts3/.libs && \
		install -d ${DESTDIR}/usr/lib/maemopaedia && \
		install libsqlite3.so.0.8.6 ${DESTDIR}/usr/lib/maemopaedia && \
		install libsqlite3.so.0 ${DESTDIR}/usr/lib/maemopaedia)

clean:
	(cd sqlite-3.6.22-fts3 && $(MAKE) clean && $(MAKE) distclean)

