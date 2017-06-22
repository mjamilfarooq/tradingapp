
add_subdirectory(akela)
add_subdirectory(cli)
add_subdirectory(avicenna)
add_subdirectory(config)
add_subdirectory(manager)
add_subdirectory(trade)
add_subdirectory(utils)
add_subdirectory(redis)
add_subdirectory(tests)


add_executable(stem stem1.cpp ${SOURCE} ${AKELA_SOURCES} ${UTILS_SOURCES})
add_executable(stem-cli STEMConsole.cpp)


target_link_libraries(stem AvicennaSuperPlus  MaddogUtils EdgeLinkx EdgeUtils EdgeDB MaddogConnection ConnectorLinkxTCP boost_atomic boost_log_setup boost_log boost_system pthread rt boost_thread boost_unit_test_framework readline XBowConnection DirectConnection EdgeDB MaddogProcessor ExchangeProcessor  libredisclient.a)


target_link_libraries(stem-cli boost_atomic boost_log_setup boost_log boost_system pthread rt boost_thread readline )


install(TARGETS stem stem-cli
		RUNTIME DESTINATION bin)
