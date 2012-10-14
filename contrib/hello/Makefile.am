ACLOCAL_AMFLAGS = -I libltdl/m4/
AM_CPPFLAGS = $(LTDLINCL) $(API_CFLAG)

SUBDIRS       = libltdl

lib_LTLIBRARIES = hello.la
hello_la_SOURCES = hello.c
hello_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -module -avoid-version -shared
