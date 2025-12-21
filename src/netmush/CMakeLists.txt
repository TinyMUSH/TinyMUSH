add_executable(netmush alloc.c api.c boolexp.c bsd.c command.c conf.c constants.h cque.c create.c db.c db_rw.c eval.c externs.h file_c.c flags.c fnhelper.c functions.c funext.c funiter.c funlist.c funmath.c funmisc.c funobj.c funstring.c funvars.c game.c help.c htab.c log.c look.c macros.h match.c mguests.c move.c nametabs.c netcommon.c object.c player.c player_c.c powers.c predicates.c prototypes.h quota.c recover.c rob.c set.c speech.c string_ansi.c string_util.c timer.c typedefs.h udb_misc.c udb_obj.c udb_ocache.c udb_ochunk.c unparse.c vattr.c version.c vt100.c walkdb.c wild.c wiz.c)
find_library(C_LIBRARY c REQUIRED)
find_library(GDBM_LIBRARY gdbm REQUIRED)
find_library(MATH_LIBRARY m REQUIRED)
find_library(PCRE_LIBRARY pcre REQUIRED)
find_library(DL_LIBRARY dl REQUIRED)
if(NOT APPLE)
    find_library(CRYPT_LIBRARY crypt REQUIRED)
endif(NOT APPLE)
target_link_libraries(netmush PRIVATE ${C_LIBRARY} ${CRYPT_LIBRARY} ${GDBM_LIBRARY} ${MATH_LIBRARY} ${NSL_LIBRARY} ${PCRE_LIBRARY} ${RESOLV_LIBRARY} ${DL_LIBRARY})
install(TARGETS netmush DESTINATION ${CMAKE_INSTALL_PREFIX}/)

## Removed temporary vt100 test and benchmark executables
