ACLOCAL_AMFLAGS = -I libltdl/m4
ACLOCAL_AUXDIR = @ac_aux_dir@
AM_CPPFLAGS = $(LTDLINCL)

SUBDIRS    = libltdl
#DIST_SUBDIRS = libltdl/

TEXT_DIR   = $(srcdir)/text
TEXT_SRC   = $(TEXT_DIR)/badsite.txt $(TEXT_DIR)/connect.txt $(TEXT_DIR)/create_reg.txt $(TEXT_DIR)/down.txt $(TEXT_DIR)/full.txt $(TEXT_DIR)/guest.txt $(TEXT_DIR)/help.txt $(TEXT_DIR)/htmlconn.txt $(TEXT_DIR)/motd.txt $(TEXT_DIR)/mushman.txt $(TEXT_DIR)/news.txt $(TEXT_DIR)/newuser.txt $(TEXT_DIR)/plushelp.txt $(TEXT_DIR)/qhelp.txt $(TEXT_DIR)/quit.txt $(TEXT_DIR)/register.txt $(TEXT_DIR)/wizhelp.txt $(TEXT_DIR)/wizmotd.txt $(TEXT_DIR)/wiznews.txt
SCRIPT_DIR = $(srcdir)/scripts
SCRIPT_SRC = $(SCRIPT_DIR)/Archive $(SCRIPT_DIR)/Backup $(SCRIPT_DIR)/Index $(SCRIPT_DIR)/Logclean $(SCRIPT_DIR)/Reconstruct $(SCRIPT_DIR)/ReportCrash $(SCRIPT_DIR)/Restore $(SCRIPT_DIR)/Startmush $(SCRIPT_DIR)/check_paths $(SCRIPT_DIR)/convert_comsys.pl $(SCRIPT_DIR)/cvt22.pl $(SCRIPT_DIR)/cvtmux.pl
SCRIPT_DAT = $(SCRIPT_DIR)/alias.conf $(SCRIPT_DIR)/compat.conf $(SCRIPT_DIR)/cvt22.sed $(SCRIPT_DIR)/cvtmux.sed $(SCRIPT_DIR)/mush.config $(SCRIPT_DIR)/mtrace.awk $(SCRIPT_DIR)/netmush.conf
EXTRA_DIST = $(TEXT_SRC) $(SCRIPT_SRC) $(SCRIPT_DAT) api.h
CLEANFILES = version.h buildnum.data

bin_PROGRAMS         = netmush mkindx logwatch slave

netmush_SOURCES = alloc.c api.c boolexp.c bsd.c command.c compat.c conf.c cque.c \
	create.c db.c db_rw.c eval.c file_c.c flags.c fnhelper.c functions.c \
	funext.c funiter.c funlist.c funmath.c funmisc.c funobj.c funstring.c \
	funvars.c game.c help.c htab.c log.c look.c match.c mguests.c move.c \
	netcommon.c object.c pcre.c player.c player_c.c powers.c predicates.c \
	quota.c rob.c set.c speech.c stringutil.c timer.c udb_ocache.c \
	udb_ochunk.c udb_obj.c udb_misc.c unparse.c vattr.c version.c \
	walkdb.c wild.c wiz.c \
	alloc.h ansi.h attrs.h autoinclude.h bitstring.h cmdtabs.h \
	command.h config.h copyright.h db.h externs.h file_c.h \
	flags.h fnproto.h functions.h getpagesize.h help.h htab.h interface.h \
	malloc.h match.h mushconf.h patchlevel.h pcre.h powers.h slave.h \
	udb.h udb_defs.h vattr.h walkdb.h
EXTRA_netmush_SOURCES  = db_sql_empty.c db_sql_msql.c db_sql_mysql.c db_sql_pgsql.c db_sql_sqlite3.c db_sql.h malloc.c mcheck.c mstats.c mtrace.c
netmush_LDFLAGS        = -export-dynamic -static
netmush_LDADD          = $(LIBOBJS) $(LIBLTDL) $(LIBADD_DL) -dlopen self $(SQL_O) $(MALLOC_O)
netmush_DEPENDENCIES   = $(LTDLDEPS) $(SQL_O) 

nodist_include_HEADERS = version.h

mkindx_SOURCES         = mkindx.c

logwatch_SOURCES       = logwatch.c

slave_SOURCES          = slave.c

BUILT_SOURCES          = version.h

if WANT_DBM_GDBM
 SUBDIRS         += dbm/gdbm/
 AM_CPPFLAGS     += -I$(srcdir)/dbm/gdbm/
 netmush_LDADD   += $(srcdir)/dbm/gdbm/.libs/libtinygdbm.a
 bin_PROGRAMS    += recover
 recover_SOURCES  = recover.c
 recover_LDADD    = $(srcdir)/dbm/gdbm/.libs/libtinygdbm.a
endif

if WANT_DBM_QDBM
 SUBDIRS         += dbm/qdbm/
 AM_CPPFLAGS     += -I$(srcdir)/dbm/qdbm/
 netmush_LDADD   += $(srcdir)/dbm/qdbm/.libs/libtinyqdbm.a
 bin_PROGRAMS    += recover
 recover_SOURCES  = recover.c
 recover_LDADD    = $(srcdir)/dbm/qdbm/.libs/libtinyqdbm.a
endif

if WANT_MODULE_COMSYS
 SUBDIRS += modules/comsys/
endif

if WANT_MODULE_MAIL
 SUBDIRS += modules/mail/
endif

if WANT_MODULE_HELLO
 SUBDIRS += modules/hello/
endif

version.h:
	touch $(srcdir)/buildnum.data
	expr 0`cat $(srcdir)/buildnum.data` + 1 > $(srcdir)/buildnum.data
	rm -f $(srcdir)/version.h
	touch $(srcdir)/version.h
	chmod +w $(srcdir)/version.h
	echo '#include "copyright.h"' >> $(srcdir)/version.h
	echo '#ifndef __VERSION_H' >> $(srcdir)/version.h
	echo '#define __VERSION_H' >> $(srcdir)/version.h
	echo "#define MUSH_BUILD_DATE \"`date`\"" >> $(srcdir)/version.h
	echo "#define MUSH_BUILD_NUM \"`cat $(srcdir)/buildnum.data`\"" >> $(srcdir)/version.h
	echo "#define MUSH_BUILD_COMPILE \"$(COMPILE)\"" >> $(srcdir)/version.h
	echo "#define MUSH_BUILD_LTCOMPILE \"$(LTCOMPILE)\"" >> $(srcdir)/version.h
	echo '#endif /* __VERSION_H */' >> $(srcdir)/version.h
	chmod -w $(srcdir)/version.h

#buildnum.data: .PHONY
#	touch $(srcdir)/buildnum.data
#	expr 0`cat $(srcdir)/buildnum.data` + 1 > $(srcdir)/buildnum.data
	
indent:
	for i in `ls *.c *.h`; do \
	astyle --style=bsd $$i; \
	done; 
	
autotools:
	shtoolize -o $(ACLOCAL_AUXDIR)/shtool all
	autoreconf -ivf
	
distclean-local:
	for i in `find -name '*~'`; do rm $$i; done
	for i in `find -name '.depend'`; do rm $$i; done
	for i in `find -name 'autom4te.cache'`; do rm -rf $$i; done

install-exec-local:
	$(mkinstalldirs) $(prefix) 
	$(install_sh_SCRIPT) $(SCRIPT_SRC) $(prefix)
	$(install_sh_DATA) $(SCRIPT_DAT) $(prefix)

install-data-local:
	$(mkinstalldirs) $(prefix)/text
	$(mkinstalldirs) $(prefix)/data
	$(mkinstalldirs) $(prefix)/backups
	$(install_sh_DATA) $(TEXT_SRC) $(prefix)/text

uninstall-local:
	for files in $(TEXT_SRC); do rm -f $(prefix)/text/`basename $$files`; done
	for files in $(SCRIPT_SRC) $(SCRIPT_DAT); do rm -f $(prefix)/`basename $$files`; done
