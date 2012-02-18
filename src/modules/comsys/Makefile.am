ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = comsys.la
comsys_la_SOURCES = comsys.c
comsys_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -dlopen self -module
