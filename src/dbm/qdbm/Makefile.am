ACLOCAL_AMFLAGS = -I libltdl/m4 -I m4

noinst_LIBRARIES             = libtinyqdbm.a
libtinyqdbm_a_SOURCES        = curia.c depot.c hovel.c myconf.c curia.h depot.h hovel.h myconf.h
nodist_libtinyqdbm_a_SOURCES = libtinyqdbm.h
BUILT_SOURCES                = libtinyqdbm.h

CLEANFILES = libtinyqdbm.h

libtinyqdbm.h: curia.h depot.h hovel.h
	rm -f $(srcdir)/libtinyqdbm.h
	touch $(srcdir)/libtinyqdbm.h
	chmod +w $(srcdir)/libtinyqdbm.h
	sed -n '/#ifndef __QDBMCOPYRIGHT_H/,/\/* __QDBMCOPYRIGHT_H *\//p' < copyright.h >> $(srcdir)/libtinyqdbm.h
	sed -n '/#ifndef __DEPOT_H/,/\/* __DEPOT_H *\//p' < depot.h >> $(srcdir)/libtinyqdbm.h
	sed -n '/#ifndef __CURIA_H/,/\/* __CURIA_H *\//p' < curia.h >> $(srcdir)/libtinyqdbm.h
	sed -n '/#ifndef __HOVEL_H/,/\/* __HOVEL_H *\//p' < hovel.h >> $(srcdir)/libtinyqdbm.h
	chmod -w $(srcdir)/libtinyqdbm.h
