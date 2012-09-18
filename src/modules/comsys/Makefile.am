ACLOCAL_AMFLAGS = -I libltdl/m4 -I m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = comsys.la
comsys_la_SOURCES = comsys.c
comsys_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -module -avoid-version -shared
