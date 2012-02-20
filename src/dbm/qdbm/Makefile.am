ACLOCAL_AMFLAGS = -I libltdl/m4
ACLOCAL_AUXDIR = @ac_aux_dir@

lib_LTLIBRARIES     = libtinyqdbm.la
libtinyqdbm_la_SOURCES = curia.c depot.c hovel.c myconf.c curia.h depot.h hovel.h myconf.h
libtinyqdbm_la_LDFLAGS = -version-info 3:0:0

nodist_include_HEADERS = libtinyqdbm.h

SUBDIRS = libltdl/

CLEANFILES = libtinyqdbm.h

libtinyqdbm.h: curia.h depot.h hovel.h .PHONY
	rm -f $(srcdir)/libtinyqdbm.h
	touch $(srcdir)/libtinyqdbm.h
	chmod +w $(srcdir)/libtinyqdbm.h
	sed -n '/#ifndef __QDBMCOPYRIGHT_H/,/\/* __QDBMCOPYRIGHT_H *\//p' < copyright.h >> $(srcdir)/libtinyqdbm.h
	sed -n '/#ifndef __DEPOT_H/,/\/* __DEPOT_H *\//p' < depot.h >> $(srcdir)/libtinyqdbm.h
	sed -n '/#ifndef __CURIA_H/,/\/* __CURIA_H *\//p' < curia.h >> $(srcdir)/libtinyqdbm.h
	sed -n '/#ifndef __HOVEL_H/,/\/* __HOVEL_H *\//p' < hovel.h >> $(srcdir)/libtinyqdbm.h
	chmod -w $(srcdir)/libtinyqdbm.h
indent:
	for i in `ls *.c *.h`; do \
	astyle --style=bsd $$i; \
	done; 
	
autotools:
	shtoolize -o $(ACLOCAL_AUXDIR)/shtool all
	autoreconf -ivf
	
rmbackup:
	for i in `find -name '*~'`; do rm $$i; done
	