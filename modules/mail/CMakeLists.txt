add_library(mail MODULE mail.c mail.h)
add_dependencies(mail netmush)
target_link_libraries(mail netmush)
install(TARGETS mail DESTINATION ${PROJECT_SOURCE_DIR}/game/modules)