ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = mail.la
mail_la_SOURCES = mail.c mail.h
mail_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -dlopen self -module
