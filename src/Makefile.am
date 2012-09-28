ACLOCAL_AMFLAGS = -I libltdl/m4 -I m4
AM_CPPFLAGS     = $(LTDLINCL)

SUBDIRS       = libltdl lib
LIBTOOL_DEPS = @LIBTOOL_DEPS@
                            

TEXT_DIR     = $(srcdir)/text
TEXT_SRC     = $(TEXT_DIR)/badsite.txt $(TEXT_DIR)/connect.txt $(TEXT_DIR)/create_reg.txt \
	$(TEXT_DIR)/down.txt $(TEXT_DIR)/full.txt $(TEXT_DIR)/guest.txt $(TEXT_DIR)/help.txt \
	$(TEXT_DIR)/htmlconn.txt $(TEXT_DIR)/motd.txt $(TEXT_DIR)/mushman.txt \
	$(TEXT_DIR)/news.txt $(TEXT_DIR)/newuser.txt $(TEXT_DIR)/plushelp.txt \
	$(TEXT_DIR)/qhelp.txt $(TEXT_DIR)/quit.txt $(TEXT_DIR)/register.txt \
	$(TEXT_DIR)/wizhelp.txt $(TEXT_DIR)/wizmotd.txt $(TEXT_DIR)/wiznews.txt
SCRIPT_DIR   = $(srcdir)/scripts
SCRIPT_SRC   = $(SCRIPT_DIR)/Archive $(SCRIPT_DIR)/Backup $(SCRIPT_DIR)/Index \
	$(SCRIPT_DIR)/Logclean $(SCRIPT_DIR)/Reconstruct $(SCRIPT_DIR)/ReportCrash \
	$(SCRIPT_DIR)/Restore $(SCRIPT_DIR)/Startmush $(SCRIPT_DIR)/check_paths \
	$(SCRIPT_DIR)/convert_comsys.pl $(SCRIPT_DIR)/cvt22.pl $(SCRIPT_DIR)/cvtmux.pl
SCRIPT_DAT   = $(SCRIPT_DIR)/alias.conf $(SCRIPT_DIR)/compat.conf $(SCRIPT_DIR)/cvt22.sed \
	$(SCRIPT_DIR)/cvtmux.sed $(SCRIPT_DIR)/mush.config $(SCRIPT_DIR)/mtrace.awk \
	$(SCRIPT_DIR)/netmush.conf
EXTRA_DIST   = $(TEXT_SRC) $(SCRIPT_SRC) $(SCRIPT_DAT) api.h m4/gnulib-cache.m4
CLEANFILES   = version.h buildnum.data tinymushapi.h
API_SRC      = typedefs.h db.h game.h alloc.h flags.h htab.h udb.h udb_defs.h mushconf.h functions.h powers.h attrs.h match.h ansi.h command.h db.h externs.h api.h

bin_PROGRAMS = netmush mkindx slave

netmush_SOURCES = alloc.c api.c boolexp.c bsd.c command.c conf.c cque.c \
	create.c db.c db_rw.c eval.c file_c.c flags.c fnhelper.c functions.c \
	funext.c funiter.c funlist.c funmath.c funmisc.c funobj.c funstring.c \
	funvars.c game.c help.c htab.c log.c look.c match.c mguests.c move.c \
	netcommon.c object.c pcre.c player.c player_c.c powers.c predicates.c \
	quota.c rob.c set.c speech.c stringutil.c timer.c udb_ocache.c \
	udb_ochunk.c udb_obj.c udb_misc.c unparse.c vattr.c version.c \
	walkdb.c wild.c wiz.c \
	alloc.h ansi.h attrs.h bitstring.h cmdtabs.h command.h game.h \
	copyright.h db.h externs.h file_c.h flags.h fnproto.h functions.h \
	getpagesize.h help.h htab.h interface.h malloc.h match.h mushconf.h \
	patchlevel.h pcre.h powers.h slave.h system.h udb.h udb_defs.h \
	vattr.h walkdb.h
EXTRA_netmush_SOURCES  = db_sql_empty.c db_sql_msql.c db_sql_mysql.c db_sql_pgsql.c db_sql_sqlite3.c db_sql.h mcheck.c mstats.c mtrace.c
netmush_LDFLAGS        = -export-dynamic
netmush_LDADD          = $(LIBOBJS) $(LIBLTDL) $(LIBADD_DL) -dlopen self $(MALLOC_O) $(FLOOR_LIBM) $(GETHOSTNAME_LIB) $(LIBSOCKET) $(LIB_SELECT) $(POW_LIBM) $(SQRT_LIBM)
netmush_DEPENDENCIES   = $(LTDLDEPS) 

include_HEADERS        = tinymushapi.h

nodist_netmush_SOURCES = version.h

mkindx_SOURCES         = mkindx.c

slave_SOURCES          = slave.c

BUILT_SOURCES          = version.h tinymushapi.h

if WANT_DBM_GDBM
 SUBDIRS         += dbm/gdbm/
 AM_CPPFLAGS     += -I$(srcdir)/dbm/gdbm/
 netmush_LDADD   += $(srcdir)/dbm/gdbm/.libs/libtinygdbm.a
 bin_PROGRAMS    += recover
 recover_SOURCES  = recover.c
 recover_LDADD    = $(srcdir)/dbm/gdbm/.libs/libtinygdbm.a $(FLOOR_LIBM) $(GETHOSTNAME_LIB) $(LIBSOCKET) $(LIB_SELECT) $(POW_LIBM) $(SQRT_LIBM)
endif

if WANT_DBM_QDBM
 SUBDIRS         += dbm/qdbm/
 AM_CPPFLAGS     += -I$(srcdir)/dbm/qdbm/
 netmush_LDADD   += $(srcdir)/dbm/qdbm/.libs/libtinyqdbm.a
 bin_PROGRAMS    += recover
 recover_SOURCES  = recover.c
 recover_LDADD    = $(srcdir)/dbm/qdbm/.libs/libtinyqdbm.a $(FLOOR_LIBM) $(GETHOSTNAME_LIB) $(LIBSOCKET) $(LIB_SELECT) $(POW_LIBM) $(SQRT_LIBM)
endif

if WANT_MODULE_COMSYS
 SUBDIRS += modules/comsys/
endif

if WANT_MODULE_MAIL
 SUBDIRS += modules/mail/
endif

if WANT_MODULE_DB_SQL
 SUBDIRS += modules/db_sql/
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

indent:
	for i in `ls *.c *.h`; do \
	astyle -A2 $$i; \
	done; 
	
testpath:
	@echo prefix : @prefix@
	@echo exec_prefix : @exec_prefix@
	@echo bindir : @bindir@
	@echo sbindir : @sbindir@
	@echo libexecdir : @libexecdir@
	@echo datarootdir : @datarootdir@
	@echo datadir : @datadir@
	@echo sysconfdir : @sysconfdir@
	@echo sharedstatedir : @sharedstatedir@
	@echo localstatedir : @localstatedir@
	@echo includedir : @includedir@
	@echo oldincludedir : @oldincludedir@
	@echo docdir : @docdir@
	@echo infodir : @infodir@
	@echo htmldir : @htmldir@
	@echo dvidir : @dvidir@
	@echo pdfdir : @pdfdir@
	@echo psdir : @psdir@
	@echo libdir : @libdir@
	@echo lispdir : @lispdir@
	@echo localedir : @localedir@
	@echo mandir : @mandir@
	@echo man1dir : @man1dir@
	@echo man2dir : @man2dir@
	@echo manext : @manext@
	@echo man1ext : @man1ext@
	@echo man2ext : @man2ext@
	@echo srcdir : @srcdir@
	
tinymushapi.h:
	rm -f $(srcdir)/tinymushapi.h
	touch $(srcdir)/tinymushapi.h
	chmod +w $(srcdir)/tinymushapi.h
	printf "#ifndef __TINYMUSHAPI_H\n#define __TINYMUSHAPI_H\n\n" >> tinymushapi.h
	printf "\n\n#include <ltdl.h>\n\n#include <netinet/in.h>\n\n" >> tinymushapi.h
	sed -n "/#ifndef __COPYRIGHT_H/,/\/* __COPYRIGHT_H *\//p" < copyright.h >> tinymushapi.h
	sed -n "/NUM_ENV_VARS/p" < config.h >> tinymushapi.h
	sed -n "/DOING_LEN/p" < config.h >> tinymushapi.h
	sed -n "/MAX_ITER_NESTING/p" < config.h >> tinymushapi.h
	sed -n "/MAX_DELIM_LEN/p" < config.h >> tinymushapi.h
	for i in $(API_SRC); \
	do \
		TMP=`echo "__$$i" | tr '.' '_' | tr '[:lower:]' '[:upper:]'`; \
		printf "\n\n/*\n * From $$i\n */\n\n" >> tinymushapi.h; \
		sed -n "/#ifndef $$TMP/,/\/* $$TMP *\//p" < $$i >> tinymushapi.h; \
	done
	printf "\n\n#endif                          /* __TINYMUSHAPI_H */\n" >> tinymushapi.h
distclean-local:
	for i in `find ./ -name '*~'`; do rm $$i; done
	for i in `find ./ -name '.depend'`; do rm $$i; done

install-exec-local:
	$(mkinstalldirs) @prefix@ 
	$(install_sh_SCRIPT) $(SCRIPT_SRC) @prefix@
	$(install_sh_DATA) $(SCRIPT_DAT) @prefix@

install-data-local:
	$(mkinstalldirs) @prefix@/text
	$(mkinstalldirs) @prefix@/data
	$(mkinstalldirs) @prefix@/backups
	$(install_sh_DATA) $(TEXT_SRC) @prefix@/text

uninstall-local:
	for files in $(TEXT_SRC); do rm -f $(prefix)/text/`basename $$files`; done
	for files in $(SCRIPT_SRC) $(SCRIPT_DAT); do rm -f $(prefix)/`basename $$files`; done

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool
