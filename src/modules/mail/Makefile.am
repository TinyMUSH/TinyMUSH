ACLOCAL_AMFLAGS = -I libltdl/m4 -I m4
AM_CPPFLAGS = $(LTDLINCL)

lib_LTLIBRARIES = mail.la
mail_la_SOURCES = mail.c mail.h
mail_la_LDFLAGS = $(LIBOBJS) $(LIBLTDL) -module -avoid-version -shared
mail_la_DEPENDENCIES = $(LTDLDEPS)
