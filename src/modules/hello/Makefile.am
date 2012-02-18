ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = hello.la
hello_la_SOURCES = hello.c
hello_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -dlopen self -module
