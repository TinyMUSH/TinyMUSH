cmake_minimum_required(VERSION 3.21.0)

project(TinyMUSH VERSION 4.0.0.0 DESCRIPTION "TinyMUSH Game Server" HOMEPAGE_URL https://github.com/TinyMUSH/ LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PACKAGE "tinymush")
set(PACKAGE_RELEASE_DATE "2023-11-25")

set(MAX_DELIM_LEN 128)
set(MAX_ITER_NESTING 1024)
set(NUM_ENV_VARS 10)

set(CMAKE_ENABLE_EXPORTS ON)

include(CTest)
enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/netmush)

find_program(TAR_BINARY tar)
configure_file(${PROJECT_SOURCE_DIR}/netmush/config.h.in ${PROJECT_SOURCE_DIR}/netmush/config.h)
configure_file(${PROJECT_SOURCE_DIR}/netmush/defaults.h.in ${PROJECT_SOURCE_DIR}/netmush/defaults.h)
configure_file(${PROJECT_SOURCE_DIR}/configs/netmush.conf.example.in ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf)
configure_file(${PROJECT_SOURCE_DIR}/configs/netmush.conf.example.in ${CMAKE_CURRENT_BINARY_DIR}/configs/netmush.conf.example)
configure_file(${PROJECT_SOURCE_DIR}/systemd/tinymush.service.in ${CMAKE_CURRENT_BINARY_DIR}/systemd/tinymush.service)

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
