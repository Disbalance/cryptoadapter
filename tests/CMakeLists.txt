add_executable(connector ../src/exchange/example/tests/connector.cpp ${COMMON_SOURCES} ${EXCHANGE_SOURCES})
target_link_libraries(connector PRIVATE ${LINK_LIBS})
