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

# Maximum parse depth to prevent stack overflow
set(MAX_BOOLEXP_PARSE_DEPTH 100)

# Memory alignment for MEMTRACK structures (bytes)
# Detect optimal alignment based on architecture if not explicitly set
if(NOT DEFINED MEMTRACK_ALIGNMENT)
    # Detect processor architecture
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
        set(MEMTRACK_ALIGNMENT 16 CACHE STRING "Memory alignment for MEMTRACK structures (16 bytes for x86_64)")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
        set(MEMTRACK_ALIGNMENT 16 CACHE STRING "Memory alignment for MEMTRACK structures (16 bytes for ARM64)")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86")
        set(MEMTRACK_ALIGNMENT 8 CACHE STRING "Memory alignment for MEMTRACK structures (8 bytes for 32-bit x86)")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|armv7")
        set(MEMTRACK_ALIGNMENT 8 CACHE STRING "Memory alignment for MEMTRACK structures (8 bytes for 32-bit ARM)")
    else()
        set(MEMTRACK_ALIGNMENT 16 CACHE STRING "Memory alignment for MEMTRACK structures (16 bytes default)")
    endif()
else()
    set(MEMTRACK_ALIGNMENT ${MEMTRACK_ALIGNMENT} CACHE STRING "Memory alignment for MEMTRACK structures (user override)")
endif()

# PCRE2 configuration
set(PCRE2_CODE_UNIT_WIDTH 8)

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

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/game" CACHE PATH "default install path" FORCE)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Build optimizations
option(TINYMUSH_USE_CCACHE "Use ccache for faster rebuilds" ON)
option(TINYMUSH_USE_LTO "Enable link-time optimization for Release builds" ON)

if(TINYMUSH_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        message("-- Using ccache: ${CCACHE_PROGRAM}")
    endif()
endif()

if(TINYMUSH_USE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message("-- Enabled link-time optimization (LTO)")
    else()
        message(WARNING "LTO not supported: ${ipo_error}")
    endif()
endif()

# Compiler-specific optimizations
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
        $<$<CONFIG:Debug>:-Og>
        $<$<CONFIG:Debug>:-g3>
    )
endif()

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
configure_file(${CMAKE_SOURCE_DIR}/src/netmush/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(${CMAKE_SOURCE_DIR}/src/configs/netmush.conf.example.in ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf)
configure_file(${CMAKE_SOURCE_DIR}/src/configs/netmush.conf.example.in ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf.example)
configure_file(${CMAKE_SOURCE_DIR}/src/systemd/tinymush.service.in ${CMAKE_CURRENT_BINARY_DIR}/systemd/tinymush.service)

include_directories(${CMAKE_SOURCE_DIR}/src/netmush)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
if(APPLE)
    include_directories(/usr/local/include /opt/homebrew/include)
endif(APPLE)

option(TINYMUSH_PRESERVE_CONFIGS "Preserve existing config files during install" ON)
option(TINYMUSH_BACKUP_CONFIGS "Backup existing configs before install when preserving" ON)

function(install_config_if_missing src filename)
    install(CODE "
        set(dst \"${CMAKE_INSTALL_PREFIX}/configs/${filename}\")
        if(NOT EXISTS \"${dst}\")
            file(INSTALL DESTINATION \"${CMAKE_INSTALL_PREFIX}/configs\" TYPE FILE FILES \"${src}\")
            message(\"-- Installed ${filename}\")
        else()
            message(\"-- Preserved existing ${filename}\")
        endif()
    ")
endfunction()

macro(install_symlink filepath sympath)
    install(CODE "
        file(CREATE_LINK ${filepath} ${sympath} RESULT clerror SYMBOLIC)
        message(\"-- Created symlink: ${sympath} -> ${filepath}\")
    ")
endmacro(install_symlink)

add_subdirectory(src/netmush)
include("${CMAKE_SOURCE_DIR}/modules.cmake")

install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/docs DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)
install(DIRECTORY DESTINATION backups)
install(DIRECTORY DESTINATION db)
install(DIRECTORY DESTINATION logs)

if(TINYMUSH_PRESERVE_CONFIGS)
    if(TINYMUSH_BACKUP_CONFIGS)
        install(CODE "
            set(cfg_dir \"${CMAKE_INSTALL_PREFIX}/configs\")
            if(EXISTS \"${cfg_dir}\")
                file(TIMESTAMP ts \"%Y%m%d%H%M%S\" UTC)
                set(backup_dir \"${CMAKE_INSTALL_PREFIX}/backups/upgrade-${ts}\")
                file(MAKE_DIRECTORY \"${backup_dir}\")
                foreach(f alias.conf compat.conf netmush.conf)
                    if(EXISTS \"${cfg_dir}/${f}\")
                        file(COPY \"${cfg_dir}/${f}\" DESTINATION \"${backup_dir}\")
                    endif()
                endforeach()
                message(\"-- Backed up existing configs to ${backup_dir}\")
            endif()
        ")
    endif()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf.example DESTINATION configs)
    install_config_if_missing("${CMAKE_SOURCE_DIR}/src/configs/alias.conf" "alias.conf")
    install_config_if_missing("${CMAKE_SOURCE_DIR}/src/configs/compat.conf" "compat.conf")
    install_config_if_missing("${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf" "netmush.conf")
else()
    install(FILES 
        ${CMAKE_SOURCE_DIR}/src/configs/alias.conf
        ${CMAKE_SOURCE_DIR}/src/configs/compat.conf
        ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf
        ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf.example
        DESTINATION configs)
endif()
install_symlink(${CMAKE_SOURCE_DIR}/game/netmush ${CMAKE_INSTALL_PREFIX}/recover)
install_symlink(${CMAKE_SOURCE_DIR}/game/netmush ${CMAKE_INSTALL_PREFIX}/dbconvert)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/scripts DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/systemd DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)

add_custom_target(install-upgrade
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
    COMMENT "Install (upgrade mode: preserve configs)"
)

add_custom_target(install-fresh
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_INSTALL_PREFIX}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
    COMMENT "Fresh install: removed ${CMAKE_INSTALL_PREFIX} then installed"
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
