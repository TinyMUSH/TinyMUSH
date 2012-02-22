ACLOCAL_AMFLAGS = -I libltdl/m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = hello.la
hello_la_SOURCES = hello.c
hello_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -dlopen self -module -avoid-version -shared
