cmake_minimum_required(VERSION 3.21.0)

project(TinyMUSH VERSION 4.0.0.0 DESCRIPTION "TinyMUSH Game Server" HOMEPAGE_URL https://github.com/TinyMUSH/ LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(Git)

function(get_git_version git_default git_version git_hash git_dirty git_date)
    message("-- Default Version: ${default}")
    if(GIT_EXECUTABLE)
        execute_process(COMMAND ${GIT_EXECUTABLE} describe --match "v[0-9]*.[0-9]*.[0-9]*" --abbrev=8
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE status
            OUTPUT_VARIABLE GIT_V
            ERROR_QUIET)
        if(${status})
            set(GIT_V "${git_default}")
        else()
            string(STRIP ${GIT_V} GIT_V)
            string(REGEX REPLACE "-[0-9]+-g" "-" GIT_V ${GIT_V})
            string(REPLACE "v" "" GIT_V ${GIT_V})
        endif()
        execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format="%ai"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE status
            OUTPUT_VARIABLE GIT_T
            ERROR_QUIET)
        if(${status})
            set(GIT_T "2000-01-01 00:00:00 -0000")
        else()
            string(REPLACE "\"" "" GIT_T ${GIT_T})
            string(STRIP ${GIT_T} GIT_T)
        endif()
        # Work out if the repository is dirty
        execute_process(COMMAND ${GIT_EXECUTABLE} update-index -q --refresh
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_QUIET
            ERROR_QUIET)
        execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD --
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_DIFF_INDEX
            ERROR_QUIET)
        string(COMPARE NOTEQUAL "${GIT_DIFF_INDEX}" "" GIT_DIRTY)
        if (${GIT_DIRTY})
            set(${git_dirty} 1 PARENT_SCOPE)
        else()
            set(${git_dirty} 0 PARENT_SCOPE)
        endif()

    else()
        set(GIT_V "${git_default}")
    endif()
    
    string(REPLACE "-" ";" GIT_V ${GIT_V})
    list (LENGTH GIT_V GIT_V_LEN)
    list(GET GIT_V 0 GIT_VERSION)
    set(${git_version} ${GIT_VERSION} PARENT_SCOPE)
    if(${GIT_V_LEN} GREATER 1)
        list(GET GIT_V 1 GIT_HASH)
        set(${git_hash} ${GIT_HASH} PARENT_SCOPE)
    endif()
    set(${git_date} ${GIT_T} PARENT_SCOPE)
endfunction()

set(PACKAGE "tinymush")

# Default values for version numbers
set(TINYMUSH_VERSION_MAJOR 4)
set(TINYMUSH_VERSION_MINOR 0) 
set(TINYMUSH_VERSION_PATCH 0)
set(TINYMUSH_VERSION_TWEAK 0)
# TINYMUSH_VERSION_STATUS
# 0 = ALPHA,
# 1 = BETA,
# 2 = CANDIDATE,
# 3 = STABLE
set(TINYMUSH_VERSION_STATUS 0)

SET(TINYMUSH_AUTHOR "TinyMUSH Development Team")
SET(TINYMUSH_CONTACT "tinymush@googlegroups.com")
SET(TINYMUSH_COPYRIGHT "Copyright (C) TinyMUSH Development Team")
SET(TINYMUSH_LICENSE "Artistic License 2.0")

get_git_version("4.0.0.0" TINYMUSH_GIT_VERSION TINYMUSH_GIT_HASH TINYMUSH_GIT_DIRTY TINYMUSH_GIT_RELEASE_DATE)

message("-- git Version: ${TINYMUSH_GIT_VERSION}")
string(REPLACE "." ";" TINYMUSH_VERSION ${TINYMUSH_GIT_VERSION})
list (LENGTH TINYMUSH_VERSION TINYMUSH_VERSION_LEN)
if(${TINYMUSH_VERSION_LEN} GREATER 0)
    list(GET TINYMUSH_VERSION 0 TINYMUSH_VERSION_MAJOR)
endif()
if(${TINYMUSH_VERSION_LEN} GREATER 1)
    list(GET TINYMUSH_VERSION 1 TINYMUSH_VERSION_MINOR)
endif()
if(${TINYMUSH_VERSION_LEN} GREATER 2)
    list(GET TINYMUSH_VERSION 2 TINYMUSH_VERSION_PATCH)
endif()
if(${TINYMUSH_VERSION_LEN} GREATER 3)
    list(GET TINYMUSH_VERSION 2 TINYMUSH_VERSION_TWEAK)
endif()
message("-- TinyMUSH Major Version: ${TINYMUSH_VERSION_MAJOR}")
message("-- TinyMUSH Minor Version: ${TINYMUSH_VERSION_MINOR}")
message("-- TinyMUSH Patch Version: ${TINYMUSH_VERSION_PATCH}")
message("-- TinyMUSH Tweak Version: ${TINYMUSH_VERSION_TWEAK}")
message("-- git Hash: ${TINYMUSH_GIT_HASH}")
message("-- git Dirty: ${TINYMUSH_GIT_DIRTY}")
message("-- git Date: ${TINYMUSH_GIT_RELEASE_DATE}")
string(TIMESTAMP TINYMUSH_BUILD_DATE "%Y-%m-%d %H:%M:%S")
message("-- Build Date: ${TINYMUSH_BUILD_DATE}")

set(MAX_DELIM_LEN 128)
set(MAX_ITER_NESTING 1024)
set(NUM_ENV_VARS 10)

set(CMAKE_ENABLE_EXPORTS ON)

include(CTest)
enable_testing()

find_program(TAR_BINARY tar)
configure_file(${PROJECT_SOURCE_DIR}/netmush/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(${PROJECT_SOURCE_DIR}/configs/netmush.conf.example.in ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf)
configure_file(${PROJECT_SOURCE_DIR}/configs/netmush.conf.example.in ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf.example)
configure_file(${PROJECT_SOURCE_DIR}/systemd/tinymush.service.in ${CMAKE_CURRENT_BINARY_DIR}/systemd/tinymush.service)

include_directories(${PROJECT_SOURCE_DIR}/netmush)
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

install(DIRECTORY ${PROJECT_SOURCE_DIR}/docs DESTINATION ${PROJECT_SOURCE_DIR}/game USE_SOURCE_PERMISSIONS)
install(DIRECTORY DESTINATION ${PROJECT_SOURCE_DIR}/game/backups)
install(DIRECTORY DESTINATION ${PROJECT_SOURCE_DIR}/game/db)
install(DIRECTORY DESTINATION ${PROJECT_SOURCE_DIR}/game/logs)
install(FILES 
    ${PROJECT_SOURCE_DIR}/configs/alias.conf
    ${PROJECT_SOURCE_DIR}/configs/compat.conf
    ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf
    ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf.example
    DESTINATION ${PROJECT_SOURCE_DIR}/game/configs)
install_symlink(${PROJECT_SOURCE_DIR}/game/netmush ${PROJECT_SOURCE_DIR}/game/recover)
install_symlink(${PROJECT_SOURCE_DIR}/game/netmush ${PROJECT_SOURCE_DIR}/game/dbconvert)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts DESTINATION ${PROJECT_SOURCE_DIR}/game USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/systemd DESTINATION ${PROJECT_SOURCE_DIR}/game USE_SOURCE_PERMISSIONS)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
