set(TARGET_NAME ${PROJECT_NAME}-algo-framework)

set(SOURCES instrument.cpp state.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-api::roq-api absl::flags)
