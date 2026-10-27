cmake_minimum_required(VERSION 3.24)
project(DoomTroopersAudit LANGUAGES NONE)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
enable_testing()
add_custom_target(check ALL
  COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests -v
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "ROM-independent tooling checks; no game runtime is built")
add_test(NAME rom_tool_tests
  COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests -v)
set_tests_properties(rom_tool_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
