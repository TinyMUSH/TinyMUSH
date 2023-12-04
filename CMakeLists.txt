cmake_minimum_required(VERSION 3.21.0)

###############################################################################
# TinyMUSH configuration
#

# Maximum length of a delimiter
set(MAX_DELIM_LEN 128)

# Maximum # of iter levels
set(MAX_ITER_NESTING 1024)

# Number of env vars (%0 et al)
set(NUM_ENV_VARS 10)

###############################################################################
# Project configuration
#

include("${CMAKE_SOURCE_DIR}/cmake/version.cmake")

get_git_tag_version("4.0.0.0" TINYMUSH_GIT_VERSION TINYMUSH_GIT_HASH TINYMUSH_GIT_DIRTY)
get_git_tag_date("2000-01-01 00:00:00 -0000" TINYMUSH_GIT_RELEASE_DATE)
SET(TINYMUSH_AUTHOR "TinyMUSH Development Team")
SET(TINYMUSH_CONTACT "tinymush@googlegroups.com")
SET(TINYMUSH_COPYRIGHT "Copyright (C) TinyMUSH Development Team")
SET(TINYMUSH_LICENSE "Artistic License 2.0")
string(TIMESTAMP TINYMUSH_BUILD_DATE "%Y-%m-%d %H:%M:%S")

###############################################################################
# TINYMUSH_RELEASE_STATUS values:
# 0 = ALPHA,
# 1 = BETA,
# 2 = CANDIDATE,
# 3 = STABLE
#
set(TINYMUSH_RELEASE_STATUS 0)

project(TinyMUSH VERSION ${TINYMUSH_GIT_VERSION} DESCRIPTION "TinyMUSH Game Server" HOMEPAGE_URL https://github.com/TinyMUSH/ LANGUAGES C CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/game" CACHE PATH "default install path" FORCE)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(TINYMUSH_GIT_DIRTY)
    message("-- Building ${PROJECT_NAME} ${TINYMUSH_GIT_VERSION}-${TINYMUSH_GIT_HASH}-dirty [${TINYMUSH_GIT_RELEASE_DATE}]")
else()
    message("-- Building ${PROJECT_NAME} ${TINYMUSH_GIT_VERSION}-${TINYMUSH_GIT_HASH} [${TINYMUSH_GIT_RELEASE_DATE}]")
endif()

message("-- CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

SET(TINYMUSH_ROOT_DIRECTORY ${CMAKE_INSTALL_PREFIX})

set(CMAKE_ENABLE_EXPORTS ON)

include(CTest)
enable_testing()

find_program(TAR_BINARY tar)
configure_file(${CMAKE_SOURCE_DIR}/netmush/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(${CMAKE_SOURCE_DIR}/configs/netmush.conf.example.in ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf)
configure_file(${CMAKE_SOURCE_DIR}/configs/netmush.conf.example.in ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf.example)
configure_file(${CMAKE_SOURCE_DIR}/systemd/tinymush.service.in ${CMAKE_CURRENT_BINARY_DIR}/systemd/tinymush.service)

include_directories(${CMAKE_SOURCE_DIR}/netmush)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
if(APPLE)
    include_directories(/usr/local/include)
endif(APPLE)

macro(install_symlink filepath sympath)
    install(CODE "
        file(CREATE_LINK ${filepath} ${sympath} RESULT clerror SYMBOLIC)
        message(\"-- Created symlink: ${sympath} -> ${filepath}\")
    ")
endmacro(install_symlink)

add_subdirectory(netmush)
add_subdirectory(modules/mail)
add_subdirectory(modules/comsys)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/docs DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)
install(DIRECTORY DESTINATION backups)
install(DIRECTORY DESTINATION db)
install(DIRECTORY DESTINATION logs)
install(FILES 
    ${CMAKE_SOURCE_DIR}/configs/alias.conf
    ${CMAKE_SOURCE_DIR}/configs/compat.conf
    ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf
    ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf.example
    DESTINATION configs)
install_symlink(${CMAKE_SOURCE_DIR}/game/netmush ${CMAKE_INSTALL_PREFIX}/recover)
install_symlink(${CMAKE_SOURCE_DIR}/game/netmush ${CMAKE_INSTALL_PREFIX}/dbconvert)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/systemd DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
