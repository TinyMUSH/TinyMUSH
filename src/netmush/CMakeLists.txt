# Database backend selection: USE_LMDB (default) or USE_GDBM
option(USE_LMDB "Use LMDB database backend" ON)
option(USE_GDBM "Use GDBM database backend" OFF)

# Ensure at least one backend is selected
if(NOT USE_LMDB AND NOT USE_GDBM)
    message(FATAL_ERROR "At least one database backend must be enabled (USE_LMDB or USE_GDBM)")
endif()

# Ensure only one backend is selected
if(USE_LMDB AND USE_GDBM)
    message(FATAL_ERROR "Only one database backend can be enabled at a time. Choose either USE_LMDB or USE_GDBM.")
endif()

# Base source files
set(NETMUSH_SOURCES
    alloc.c ansi.c ansi.h api.c boolexp.c bsd.c bsd_conn.c bsd_dns.c 
    bsd_io.c bsd_sig.c bsd_socket.c command.c conf.c constants.h 
    cque.c create.c db_attributes.c db_filehelpers.c db_flatfile.c db_fwdlist.c 
    db_objects.c db_propdir.c db_serialization.c eval.c externs.h file_c.c flags.c fnhelper.c 
    functions.c funext.c funiter.c funlist.c funmath.c funmisc.c funobj.c 
    funstring.c funvars.c game.c help.c htab.c log.c look.c macros.h match.c 
    mguests.c move.c nametabs.c netcommon.c object.c player.c player_c.c 
    powers.c predicates.c prototypes.h quota.c rob.c set.c speech.c 
    string_util.c timer.c typedefs.h 
    unparse.c vattr.c version.c walkdb.c wild.c wiz.c
    db_storage.c db_storage.h
        command_init.c
        command_access.c
        command_dispatch.c
        command_list.c
        # command_config.c
        # command_info.c
)

# Add backend-specific source files
if(USE_LMDB)
    list(APPEND NETMUSH_SOURCES db_backend_lmdb.c)
    add_compile_definitions(USE_LMDB)
    message(STATUS "Database backend: LMDB")
endif()

if(USE_GDBM)
    list(APPEND NETMUSH_SOURCES db_backend_gdbm.c)
    add_compile_definitions(USE_GDBM)
    message(STATUS "Database backend: GDBM")
endif()

add_executable(netmush ${NETMUSH_SOURCES})

find_library(C_LIBRARY c REQUIRED)
find_library(MATH_LIBRARY m REQUIRED)
find_library(PCRE_LIBRARY pcre2-8 REQUIRED)
find_library(DL_LIBRARY dl REQUIRED)

# Backend-specific libraries
if(USE_LMDB)
    find_library(LMDB_LIBRARY lmdb REQUIRED)
endif()

if(USE_GDBM)
    find_library(GDBM_LIBRARY gdbm REQUIRED)
endif()

if(NOT APPLE)
    find_library(CRYPT_LIBRARY crypt REQUIRED)
endif(NOT APPLE)

# Link libraries based on backend
set(NETMUSH_LIBRARIES ${C_LIBRARY} ${CRYPT_LIBRARY} ${MATH_LIBRARY} ${NSL_LIBRARY} ${PCRE_LIBRARY} ${RESOLV_LIBRARY} ${DL_LIBRARY})

if(USE_LMDB)
    list(APPEND NETMUSH_LIBRARIES ${LMDB_LIBRARY})
endif()

if(USE_GDBM)
    list(APPEND NETMUSH_LIBRARIES ${GDBM_LIBRARY})
endif()

target_link_libraries(netmush PRIVATE ${NETMUSH_LIBRARIES})
install(TARGETS netmush DESTINATION ${CMAKE_INSTALL_PREFIX}/)

# Database converter utility (requires both backends); disabled when builds are mutually exclusive.
message(STATUS "dbconvert skipped (LMDB/GDBM builds are mutually exclusive)")

## Removed temporary vt100 test and benchmark executables
