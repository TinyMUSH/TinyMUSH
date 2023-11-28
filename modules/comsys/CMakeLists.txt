add_library(comsys MODULE comsys.c)
add_dependencies(comsys netmush)
install(TARGETS comsys DESTINATION ${PROJECT_SOURCE_DIR}/game/modules)
