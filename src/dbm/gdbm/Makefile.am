ACLOCAL_AMFLAGS = -I libltdl/m4 -I m4

noinst_LIBRARIES              = libtinygdbm.a
libtinygdbm_a_SOURCES         = gdbmbucket.c gdbmfalloc.c gdbmfindkey.c gdbmclose.c gdbmdelete.c gdbmerrno.c gdbmexists.c gdbmfdesc.c gdbmfetch.c gdbmopen.c gdbmreorg.c gdbmseq.c gdbmsetopt.c gdbmstore.c gdbmsync.c gdbmglobal.c gdbmhash.c gdbmupdate.c gdbmversion.c gdbmextern.h gdbmconst.h gdbmdefs.h gdbmerrno.h gdbmproto.h gdbmsystems.h gdbmcopyright.h
nodist_libtinygdbm_a_SOURCES  = libtinygdbm.h
BUILT_SOURCES                 = libtinygdbm.h

EXTRA_DIST = libtinygdbm.head.h libtinygdbm.foot.h 
CLEANFILES = libtinygdbm.h

libtinygdbm.h: libtinygdbm.head.h gdbmerrno.h gdbmconst.h gdbmdefs.h gdbmerrno.h gdbmextern.h gdbmproto.h libtinygdbm.foot.h
	rm -f $(srcdir)/libtinygdbm.h
	touch $(srcdir)/libtinygdbm.h
	chmod +w libtinygdbm.h
	cat $(srcdir)/libtinygdbm.head.h >> libtinygdbm.h
	sed -n '/#ifndef __GDBMCOPYRIGHT_H/,/\/* __GDBMCOPYRIGHT_H *\//p' < gdbmcopyright.h >> libtinygdbm.h
	printf "\n/*\n * from gdbmconst.h\n */\n\n">> libtinygdbm.h
	sed -n '/#ifndef __GDBMCONST_H/,/\/* __GDBMCONST_H *\//p' < gdbmconst.h >> libtinygdbm.h
	printf "\n/*\n * from gdbmdefs.h\n */\n\n">> libtinygdbm.h
	sed -n '/#ifndef __GDBMDEFS_H/,/\/* __GDBMDEFS_H *\//p' < gdbmdefs.h >> libtinygdbm.h
	printf "\n/*\n * from gdbmerrno.h\n */\n\n">> libtinygdbm.h
	sed -n '/#ifndef __GDBMERRNO_H/,/\/* __GDBMERRNO_H *\//p' < gdbmerrno.h >> libtinygdbm.h
	printf "\n/*\n * from gdbmextern.h\n */\n\n">> libtinygdbm.h
	sed -n '/#ifndef __GDBMEXTERN_H/,/\/* __GDBMEXTERN_H *\//p' < gdbmextern.h >> libtinygdbm.h
	cat $(srcdir)/libtinygdbm.foot.h >> libtinygdbm.h
	chmod -w libtinygdbm.h
