
add_executable(order_sending order_sending.cpp ${AKELA_SOURCES} ${UTILS_SOURCES})
add_definitions(-DUNIT_TEST )
add_executable(position_table position_table.cpp)

target_link_libraries(order_sending MaddogUtils EdgeLinkx EdgeUtils EdgeDB MaddogConnection ConnectorLinkxTCP boost_atomic boost_log_setup boost_log boost_system pthread rt boost_thread boost_unit_test_framework readline XBowConnection DirectConnection EdgeDB MaddogProcessor ExchangeProcessor)
target_link_libraries(position_table  boost_atomic boost_system pthread rt boost_log_setup boost_log boost_thread boost_unit_test_framework)

install(TARGETS order_sending
		RUNTIME DESTINATION ../bin)
install(TARGETS position_table
		RUNTIME DESTINATION ../bin)		
