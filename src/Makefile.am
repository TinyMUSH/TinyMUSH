AM_CPPFLAGS				= $(LTDLINCL)
SUBDIRS					= libltdl
LIBTOOL_DEPS			= @LIBTOOL_DEPS@

AUTOMAKE_OPTIONS 		= subdir-objects

netmush_C				= alloc.c api.c boolexp.c bsd.c command.c conf.c cque.c create.c db.c db_rw.c eval.c file_c.c flags.c fnhelper.c functions.c funext.c funiter.c funlist.c funmath.c funmisc.c funobj.c funstring.c funvars.c game.c help.c htab.c log.c look.c match.c mguests.c move.c nametabs.c netcommon.c object.c player.c player_c.c powers.c predicates.c quota.c recover.c rob.c set.c speech.c stringutil.c timer.c udb_misc.c udb_obj.c udb_ocache.c udb_ochunk.c unparse.c vattr.c version.c walkdb.c wild.c wiz.c
netmush_H				= alloc.h api.h attrs.h cmdtabs.h command.h copyright.h db.h externs.h file_c.h flags.h fnproto.h functions.h game.h help.h htab.h interface.h log.h match.h mushconf.h nametabs.h player.h player_c.h powers.h stringutil.h system.h typedefs.h udb_defs.h udb.h udb_ocache.h vattr.h walkdb.h wiz.h
netmush_SOURCES			= $(netmush_C) $(netmush_H)
netmush_LDFLAGS			= -export-dynamic
netmush_SKEL			= defaults.h.in
netmush_LDADD			= $(LIBOBJS) $(LIBLTDL) $(LIBADD_DL) -dlopen self $(MALLOC_O) $(FLOOR_LIBM) $(GETHOSTNAME_LIB) $(LIBSOCKET) $(LIB_SELECT) $(POW_LIBM) $(SQRT_LIBM)
netmush_DEPENDENCIES	= $(LTDLDEPS) 
nodist_netmush_SOURCES	= config.h defaults.h prototypes.h tinymushapi.h version.h

slave_C					= slave.c
slave_H					= slave.h
slave_SOURCES			= $(slave_C) $(slave_H)

SHARE_DIR				= share
SHARE_SRC				= $(SHARE_DIR)/badsite.txt $(SHARE_DIR)/connect.txt $(SHARE_DIR)/create_reg.txt $(SHARE_DIR)/down.txt $(SHARE_DIR)/full.txt $(SHARE_DIR)/guest.txt $(SHARE_DIR)/help.txt $(SHARE_DIR)/htmlconn.txt $(SHARE_DIR)/motd.txt $(SHARE_DIR)/mushman.txt $(SHARE_DIR)/news.txt $(SHARE_DIR)/newuser.txt $(SHARE_DIR)/plushelp.txt $(SHARE_DIR)/qhelp.txt $(SHARE_DIR)/quit.txt $(SHARE_DIR)/register.txt $(SHARE_DIR)/wizhelp.txt $(SHARE_DIR)/wizmotd.txt $(SHARE_DIR)/wiznews.txt
	
ETC_DIR					= etc
ETC_SKEL				= $(ETC_DIR)/netmush.conf.example.in
ETC_SRC					= $(ETC_DIR)/alias.conf $(ETC_DIR)/compat.conf
ETC_BUILD				= $(ETC_DIR)/netmush.conf.example

prototypes_SKEL			= prototypes.h.in

SCRIPT_DIR				= scripts
SCRIPT_SKEL				= $(SCRIPT_DIR)/Startmush.in
SCRIPT_SRC				= $(SCRIPT_DIR)/Archive $(SCRIPT_DIR)/Backup $(SCRIPT_DIR)/Index $(SCRIPT_DIR)/Logclean $(SCRIPT_DIR)/Reconstruct $(SCRIPT_DIR)/ReportCrash $(SCRIPT_DIR)/Restore $(SCRIPT_DIR)/check_paths $(SCRIPT_DIR)/convert_comsys.pl $(SCRIPT_DIR)/cvt22.pl $(SCRIPT_DIR)/cvtmux.pl
SCRIPT_DAT				= $(SCRIPT_DIR)/cvt22.sed $(SCRIPT_DIR)/cvtmux.sed $(SCRIPT_DIR)/mush.config $(SCRIPT_DIR)/mtrace.awk
SCRIPT_BUILD			= $(SCRIPT_DIR)/Startmush

version_SKEL			= version.h.in

EXTRA_DIST				= libltdl/config/shtool $(netmush_SKEL) $(ETC_SKEL) $(ETC_SRC) $(SHARE_SRC) $(SCRIPT_SKEL) $(SCRIPT_SRC) $(SCRIPT_DAT) api.h $(version_SKEL) $(prototypes_SKEL)

CLEANFILES				= defaults.h prototypes.h tinymushapi.h version.h $(ETC_DIR)/netmush.conf.example $(SCRIPT_DIR)/Startmush

API_SRC					= typedefs.h db.h game.h alloc.h flags.h htab.h udb.h udb_defs.h mushconf.h functions.h powers.h attrs.h match.h command.h api.h interface.h externs.h stringutil.h prototypes.h

BUILT_SOURCES			= prototypes.h version.h tinymushapi.h defaults.h netmush.conf.example Startmush

nodist_include_HEADERS	= tinymushapi.h

bin_PROGRAMS			= netmush slave

CPROTO_INCLUDES			=

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
	
netmush.conf.example: $(ETC_DIR)/netmush.conf.example.in
	$(do_subst) <$(ETC_DIR)/netmush.conf.example.in >$(ETC_DIR)/netmush.conf.example
	chmod +w $(ETC_DIR)/netmush.conf.example
	
defaults.h: defaults.h.in
	$(do_subst) <defaults.h.in >defaults.h
	chmod +w defaults.h
	
Startmush: $(SCRIPT_DIR)/Startmush.in
	$(do_subst) <$(SCRIPT_DIR)/Startmush.in >$(SCRIPT_DIR)/Startmush
	chmod +x $(SCRIPT_DIR)/Startmush
	
#.PHONY:version.h
	
prototypes.h: defaults.h version.h prototypes.h.in
	rm -f prototypes.h
	touch prototypes.h
	cp prototypes.h.in prototypes.h
	for i in $(netmush_C); do echo "$$($(with_cproto_prog) $(CPROTO_INCLUDES) -e -f 3 $$i 2>/dev/null)" >> prototypes.h; echo '' >> prototypes.h; done
	echo '#endif /* __PROTOTYPES_H */' >> prototypes.h
	chmod -w prototypes.h

version.h: version.h.in
	rm -f version.h
	$(SED) -e 's|[@]COMPILE@|$(COMPILE)|g' \
	       -e's|[@]LTCOMPILE@|$(LTCOMPILE)|g' < version.h.in > version.h
	touch version.h
	chmod -w version.h

tinymushapi.h: $(API_SRC)
	rm -f tinymushapi.h
	touch tinymushapi.h
	chmod +w tinymushapi.h
	printf "#ifndef __TINYMUSHAPI_H\n#define __TINYMUSHAPI_H\n\n" >> tinymushapi.h
	printf "\n\n#include <ltdl.h>\n\n#include <netinet/in.h>\n\n" >> tinymushapi.h
	$(SED) -n "/#ifndef __COPYRIGHT_H/,/\/* __COPYRIGHT_H *\//p" < copyright.h >> tinymushapi.h
	$(SED) -n "/NUM_ENV_VARS/p" < config.h >> tinymushapi.h
	$(SED) -n "/MAX_ITER_NESTING/p" < config.h >> tinymushapi.h
	$(SED) -n "/MAX_DELIM_LEN/p" < config.h >> tinymushapi.h
	for i in $(API_SRC); \
	do \
		TMP=`echo "__$$i" | $(SED) "s/src\///g" | tr '.' '_' | tr '[:lower:]' '[:upper:]'`; \
		TMP1=`echo "$$i" | $(SED) "s/src\///g"`; \
		printf "\n\n/*\n * From $$TMP1\n */\n\n" >> tinymushapi.h; \
		$(SED) -n "/#ifndef $$TMP/,/\/* $$TMP *\//p" < $$i >> tinymushapi.h; \
	done
	printf "\n\n#endif                          /* __TINYMUSHAPI_H */\n" >> tinymushapi.h

install-exec-local:
	$(mkinstalldirs) @prefix@
	$(mkinstalldirs) @libexecdir@/@with_install_suffix@
	$(install_sh_SCRIPT) $(SCRIPT_SRC) @libexecdir@/@with_install_suffix@
	$(install_sh_SCRIPT) $(SCRIPT_BUILD) @libexecdir@/@with_install_suffix@
	$(install_sh_DATA) $(SCRIPT_DAT) @libexecdir@/@with_install_suffix@

install-exec-hook:
	cd @bindir@ && $(LN_S) -f netmush dbconvert && $(LN_S) -f netmush recover

install-data-local:
	$(mkinstalldirs) @sysconfdir@
	$(mkinstalldirs) @datarootdir@
	$(mkinstalldirs) @localstatedir@/run/@with_install_suffix@
	$(mkinstalldirs) @localstatedir@/db/@with_install_suffix@
	$(mkinstalldirs) @localstatedir@/backups/@with_install_suffix@
	$(mkinstalldirs) @localstatedir@/log/@with_install_suffix@
	$(install_sh_DATA) $(SHARE_SRC) @datarootdir@
	$(install_sh_DATA) $(ETC_SRC) @sysconfdir@
	$(install_sh_DATA) $(ETC_BUILD) @sysconfdir@
	@for i in $(ETC_BUILD); \
	do \
		SKEL_CONF="`$(with_basename_prog) $$i`"; \
		USER_CONF="@sysconfdir@/`echo $$i | $(with_basename_prog) $$($(SED) s/".example"//)`"; \
		if [ ! -f "$$USER_CONF" ]; then \
			cp @sysconfdir@/$$SKEL_CONF $$USER_CONF; \
			echo "No $$USER_CONF found. Default configuration file created."; \
		fi; \
	done

uninstall-local:
	for files in $(SHARE_SRC); do rm -f $(prefix)/text/`basename $$files`; done
	for files in $(SCRIPT_SRC) $(SCRIPT_DAT); do rm -f $(prefix)/`basename $$files`; done
	rm -f @bindir@/dbconvert 
	rm -f @bindir@/recover
