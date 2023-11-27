add_library(mail MODULE mail.c mail.h)
install(TARGETS mail DESTINATION ${PROJECT_SOURCE_DIR}/game/modules)