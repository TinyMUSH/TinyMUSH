add_library(comsys MODULE comsys.c)
add_dependencies(comsys netmush)
target_link_libraries(comsys netmush)
install(TARGETS comsys DESTINATION ${PROJECT_SOURCE_DIR}/game/modules)
