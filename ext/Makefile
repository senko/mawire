
all:
	(cd sqlite-3.6.22-fts3 && \
		./configure --libdir=/opt/mawire/lib && \
		$(MAKE))

install:
	(cd sqlite-3.6.22-fts3/.libs && \
		install -d ${DESTDIR}/opt/mawire/lib && \
		install libsqlite3.so.0.8.6 ${DESTDIR}/opt/mawire/lib && \
		install libsqlite3.so.0 ${DESTDIR}/opt/mawire/lib)

clean:
	(cd sqlite-3.6.22-fts3 && $(MAKE) clean && $(MAKE) distclean)

