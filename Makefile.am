
ACLOCAL_AMFLAGS = -I src/libltdl/m4

AM_CPPFLAGS     = $(LTDLINCL)

SUBDIRS         = src

EXTRA_DIST      = contrib/hello CONVERTING ChangeLog.Alpha ChangeLog.Beta ChangeLog.History FAQ MODULES
	
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

astyle:
	astyle --recursive --mode=c --delete-empty-lines --break-blocks --pad-oper --pad-paren --convert-tabs --indent=spaces --style=kr $$i "./*.c"
	astyle --recursive --mode=c --delete-empty-lines --break-blocks --pad-oper --pad-paren --convert-tabs --indent=spaces --style=kr $$i "./*.h"

distclean-local:
	for i in `find ./ -name '*~'`; do rm $$i; done
	for i in `find ./ -name '.depend'`; do rm $$i; done
	for i in `find ./ -name 'autom4te.cache'`; do rm -rf $$i; done
	for i in `find ./ -name '*.orig'`; do rm -rf $$i; done

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

