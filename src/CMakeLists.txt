add_executable(cli main.cpp)
target_link_libraries(cli PRIVATE
  jav::jav
)

set_target_properties(cli PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_compile_definitions(cli PRIVATE CLI_VERSION="${PROJECT_VERSION}")
install(TARGETS cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
