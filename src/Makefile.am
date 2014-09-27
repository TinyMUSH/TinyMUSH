AM_CPPFLAGS		= $(LTDLINCL)
SUBDIRS			= libltdl
LIBTOOL_DEPS		= @LIBTOOL_DEPS@

AUTOMAKE_OPTIONS 	= subdir-objects

netmush_SOURCES		= alloc.c api.c boolexp.c bsd.c command.c conf.c cque.c create.c db.c db_rw.c eval.c file_c.c flags.c fnhelper.c functions.c funext.c funiter.c funlist.c funmath.c funmisc.c funobj.c funstring.c funvars.c game.c help.c htab.c log.c look.c match.c mguests.c move.c netcommon.c object.c player.c player_c.c powers.c predicates.c quota.c rob.c set.c speech.c stringutil.c timer.c udb_misc.c udb_obj.c udb_ocache.c udb_ochunk.c unparse.c vattr.c version.c walkdb.c wild.c wiz.c alloc.h ansi.h attrs.h bitstring.h cmdtabs.h command.h copyright.h db.h externs.h file_c.h flags.h fnproto.h functions.h game.h help.h htab.h interface.h match.h mushconf.h powers.h slave.h system.h typedefs.h udb.h udb_defs.h vattr.h walkdb.h
netmush_LDFLAGS		= -export-dynamic
netmush_SKEL		= defaults.h.in
netmush_LDADD		= $(LIBOBJS) $(LIBLTDL) $(LIBADD_DL) -dlopen self $(MALLOC_O) $(FLOOR_LIBM) $(GETHOSTNAME_LIB) $(LIBSOCKET) $(LIB_SELECT) $(POW_LIBM) $(SQRT_LIBM)
netmush_DEPENDENCIES	= $(LTDLDEPS) 
nodist_netmush_SOURCES	= version.h defaults.h

recover_SOURCES		= recover.c
recover_LDADD		= $(LIBOBJS) $(LIBLTDL) $(LIBADD_DL) $(MALLOC_O) $(FLOOR_LIBM) $(GETHOSTNAME_LIB) $(LIBSOCKET) $(LIB_SELECT) $(POW_LIBM) $(SQRT_LIBM)

slave_SOURCES		= slave.c

SHARE_DIR		= share
SHARE_SRC		= $(SHARE_DIR)/badsite.txt $(SHARE_DIR)/connect.txt $(SHARE_DIR)/create_reg.txt $(SHARE_DIR)/down.txt $(SHARE_DIR)/full.txt $(SHARE_DIR)/guest.txt $(SHARE_DIR)/help.txt $(SHARE_DIR)/htmlconn.txt $(SHARE_DIR)/motd.txt $(SHARE_DIR)/mushman.txt $(SHARE_DIR)/news.txt $(SHARE_DIR)/newuser.txt $(SHARE_DIR)/plushelp.txt $(SHARE_DIR)/qhelp.txt $(SHARE_DIR)/quit.txt $(SHARE_DIR)/register.txt $(SHARE_DIR)/wizhelp.txt $(SHARE_DIR)/wizmotd.txt $(SHARE_DIR)/wiznews.txt
	
ETC_DIR			= etc
ETC_SKEL		= $(ETC_DIR)/netmush.conf.in
ETC_SRC			= $(ETC_DIR)/alias.conf $(ETC_DIR)/compat.conf
ETC_BUILD		= $(ETC_DIR)/netmush.conf

SCRIPT_DIR		= scripts
SCRIPT_SKEL		= $(SCRIPT_DIR)/Startmush.in
SCRIPT_SRC		= $(SCRIPT_DIR)/Archive $(SCRIPT_DIR)/Backup $(SCRIPT_DIR)/Index $(SCRIPT_DIR)/Logclean $(SCRIPT_DIR)/Reconstruct $(SCRIPT_DIR)/ReportCrash $(SCRIPT_DIR)/Restore $(SCRIPT_DIR)/check_paths $(SCRIPT_DIR)/convert_comsys.pl $(SCRIPT_DIR)/cvt22.pl $(SCRIPT_DIR)/cvtmux.pl
SCRIPT_DAT		= $(SCRIPT_DIR)/cvt22.sed $(SCRIPT_DIR)/cvtmux.sed $(SCRIPT_DIR)/mush.config $(SCRIPT_DIR)/mtrace.awk
SCRIPT_BUILD		= $(SCRIPT_DIR)/Startmush

EXTRA_DIST		= libltdl/config/shtool $(netmush_SKEL) $(ETC_SKEL) $(ETC_SRC) $(SHARE_SRC) $(SCRIPT_SKEL) $(SCRIPT_SRC) $(SCRIPT_DAT) api.h
CLEANFILES		= version.h defaults.h tinymushapi.h $(ETC_DIR)/netmush.conf $(SCRIPT_DIR)/Startmush

API_SRC			= typedefs.h db.h game.h alloc.h flags.h htab.h udb.h udb_defs.h mushconf.h functions.h powers.h attrs.h match.h ansi.h command.h db.h api.h interface.h externs.h

BUILT_SOURCES		= version.h tinymushapi.h defaults.h netmush.conf Startmush

nodist_include_HEADERS	= tinymushapi.h

bin_PROGRAMS		= netmush slave recover

if WANT_DBM_GDBM
 SUBDIRS		+= dbm/gdbm/
 AM_CPPFLAGS		+= -I dbm/gdbm/
 netmush_LDADD		+= dbm/gdbm/libtinygdbm.a
 recover_LDADD		+= dbm/gdbm/libtinygdbm.a
endif

if WANT_DBM_QDBM
 SUBDIRS		+= dbm/qdbm/
 AM_CPPFLAGS		+= -I dbm/qdbm/
 netmush_LDADD		+= dbm/qdbm/libtinyqdbm.a
 recover_LDADD		+= dbm/qdbm/libtinyqdbm.a
endif

if WANT_MODULE_COMSYS
 SUBDIRS		+= modules/comsys/
endif

if WANT_MODULE_MAIL
 SUBDIRS		+= modules/mail/
endif

if WANT_MODULE_DB_SQL
 SUBDIRS		+= modules/db_sql/
endif

astyle:
	for i in $(netmush_SOURCES) $(recover_SOURCES) $(slave_SOURCES) ; do \
	astyle --mode=c --delete-empty-lines --break-blocks --pad-oper --pad-paren --convert-tabs --indent=spaces --style=kr $$i; \
	done; 

do_subst = $(SED) \
	-e 's|[@]with_tar_prog@|$(with_tar_prog)|g' \
	-e 's|[@]with_compress_prog@|$(with_compress_prog)|g' \
	-e 's|[@]prefix@|$(prefix)|g' \
	-e 's|[@]exec_prefix@|$(exec_prefix)|g' \
	-e 's|[@]bindir@|$(bindir)|g' \
	-e 's|[@]datarootdir@|$(datarootdir)|g' \
	-e 's|[@]with_install_suffix@|$(with_install_suffix)|g' \
	-e 's|[@]with_install_suffix@|$(with_install_suffix)|g' \
	-e 's|[@]sysconfdir@|$(sysconfdir)|g' \
	-e 's|[@]sysconfdir@|$(sysconfdir)|g' \
	-e 's|[@]localstatedir@|$(localstatedir)|g' \
	-e 's|[@]localstatedir@|$(localstatedir)|g' \
	-e 's|[@]libdir@|$(libdir)|g' \
	-e 's|[@]includedir@|$(includedir)|g' \
	-e 's|[@]libexecdir@|$(libexecdir)|g'
	
netmush.conf: $(ETC_DIR)/netmush.conf.in
	$(do_subst) <$(ETC_DIR)/netmush.conf.in >$(ETC_DIR)/netmush.conf
	chmod +w $(ETC_DIR)/netmush.conf
	
defaults.h: defaults.h.in
	$(do_subst) <defaults.h.in >defaults.h
	chmod +w defaults.h
	
Startmush: $(SCRIPT_DIR)/Startmush.in
	$(do_subst) <$(SCRIPT_DIR)/Startmush.in >$(SCRIPT_DIR)/Startmush
	chmod +x $(SCRIPT_DIR)/Startmush
	
version.h:
	rm -f version.h
	touch version.h
	chmod +w version.h
	echo '#include "copyright.h"' >> version.h
	echo '#ifndef __VERSION_H' >> version.h
	echo '#define __VERSION_H' >> version.h
	echo "#define MUSH_BUILD_DATE \"`date`\"" >> version.h
	echo "#define MUSH_BUILD_COMPILE \"$(COMPILE)\"" >> version.h
	echo "#define MUSH_BUILD_LTCOMPILE \"$(LTCOMPILE)\"" >> version.h
	echo '#endif /* __VERSION_H */' >> version.h
	chmod -w version.h

tinymushapi.h:
	rm -f tinymushapi.h
	touch tinymushapi.h
	chmod +w tinymushapi.h
	printf "#ifndef __TINYMUSHAPI_H\n#define __TINYMUSHAPI_H\n\n" >> tinymushapi.h
	printf "\n\n#include <ltdl.h>\n\n#include <netinet/in.h>\n\n" >> tinymushapi.h
	sed -n "/#ifndef __COPYRIGHT_H/,/\/* __COPYRIGHT_H *\//p" < copyright.h >> tinymushapi.h
	sed -n "/NUM_ENV_VARS/p" < config.h >> tinymushapi.h
	sed -n "/DOING_LEN/p" < config.h >> tinymushapi.h
	sed -n "/MAX_ITER_NESTING/p" < config.h >> tinymushapi.h
	sed -n "/MAX_DELIM_LEN/p" < config.h >> tinymushapi.h
	for i in $(API_SRC); \
	do \
		TMP=`echo "__$$i" | sed "s/src\///g" | tr '.' '_' | tr '[:lower:]' '[:upper:]'`; \
		TMP1=`echo "$$i" |sed "s/src\///g"`; \
		printf "\n\n/*\n * From $$TMP1\n */\n\n" >> tinymushapi.h; \
		sed -n "/#ifndef $$TMP/,/\/* $$TMP *\//p" < $$i >> tinymushapi.h; \
	done
	printf "\n\n#endif                          /* __TINYMUSHAPI_H */\n" >> tinymushapi.h

install-exec-local:
	$(mkinstalldirs) @prefix@
	$(mkinstalldirs) @libexecdir@/@with_install_suffix@
	$(install_sh_SCRIPT) $(SCRIPT_SRC) @libexecdir@/@with_install_suffix@
	$(install_sh_SCRIPT) $(SCRIPT_BUILD) @libexecdir@/@with_install_suffix@
	$(install_sh_DATA) $(SCRIPT_DAT) @libexecdir@/@with_install_suffix@

install-data-local:
	$(mkinstalldirs) @sysconfdir@
	$(mkinstalldirs) @datarootdir@
	$(mkinstalldirs) @localstatedir@/run/@with_install_suffix@
	$(mkinstalldirs) @localstatedir@/db/@with_install_suffix@
	$(mkinstalldirs) @localstatedir@/backups/@with_install_suffix@
	$(mkinstalldirs) @localstatedir@/log/@with_install_suffix@
	$(install_sh_DATA) $(SHARE_SRC) @datarootdir@
	$(install_sh_DATA) $(ETC_SRC) @sysconfdir@
	$(install_sh_DATA) $(ETC_SRC) @sysconfdir@
	$(install_sh_DATA) $(ETC_BUILD) @sysconfdir@

uninstall-local:
	for files in $(SHARE_SRC); do rm -f $(prefix)/text/`basename $$files`; done
	for files in $(SCRIPT_SRC) $(SCRIPT_DAT); do rm -f $(prefix)/`basename $$files`; done

