cmake_minimum_required(VERSION 3.24)
project(DoomBootProbe LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(RECOMP_ROOT "${ROOT}/local/segagenesisrecomp" CACHE PATH "Pinned experimental framework")
set(DOOM_ROM "" CACHE FILEPATH "Local ROM; never copied or packaged")
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
if(DOOM_ROM)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${ROOT}/tools/rom_tool.py" validate "${DOOM_ROM}"
  COMMAND_ERROR_IS_FATAL ANY)
endif()
execute_process(COMMAND git -C "${RECOMP_ROOT}" rev-parse HEAD OUTPUT_VARIABLE REV OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
if(NOT REV STREQUAL "f422ad52a04b9d253c0e9cbcdb3476aabd563ae8")
  message(FATAL_ERROR "Framework revision differs from audited pin")
endif()
set(RUNNER "${RECOMP_ROOT}/runner")
set(SDLROOT "${RUNNER}/external/SDL2/SDL2-2.28.5")
find_package(SDL2 REQUIRED CONFIG NO_DEFAULT_PATH PATHS "${SDLROOT}/cmake")
include("${RECOMP_ROOT}/cmake/GenesisRecompRunner.cmake")
add_subdirectory("${RECOMP_ROOT}/recompiler" translator)
enable_testing()
add_executable(TrapGapRegression trap_regression.c "${RUNNER}/m68k_interp.c"
  "${RECOMP_ROOT}/recompiler/src/m68k_decoder.c" "${RECOMP_ROOT}/recompiler/src/rom_parser.c")
target_include_directories(TrapGapRegression PRIVATE "${RUNNER}" "${RUNNER}/include"
  "${RECOMP_ROOT}/recompiler/src")
add_test(NAME known_cartridge_trap_gap COMMAND TrapGapRegression)
set(TRAP_DIR "${CMAKE_CURRENT_BINARY_DIR}/trap-generated")
file(MAKE_DIRECTORY "${TRAP_DIR}")
set(TRAP_SOURCES "${TRAP_DIR}/trap_module.c")
foreach(N RANGE 0 31)
  if(N LESS 10)
    set(N "0${N}")
  endif()
  list(APPEND TRAP_SOURCES "${TRAP_DIR}/trap_part${N}.c")
endforeach()
add_custom_command(OUTPUT ${TRAP_SOURCES} "${TRAP_DIR}/fixture.bin"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/make_trap_fixture.py" "${TRAP_DIR}/fixture.bin"
  COMMAND "$<TARGET_FILE:M68kStepRecomp>" "${TRAP_DIR}/fixture.bin" "${TRAP_DIR}" trap 0 0x200 0x204 0x300
  DEPENDS M68kStepRecomp make_trap_fixture.py VERBATIM)
add_executable(TrapNativeProof trap_native.c ${TRAP_SOURCES}
  "${RECOMP_ROOT}/external/m68k-recomp-core/runtime/m68k_step.c")
target_include_directories(TrapNativeProof PRIVATE "${TRAP_DIR}"
  "${RECOMP_ROOT}/external/m68k-recomp-core/runtime")
add_test(NAME synthetic_native_trap_rte COMMAND TrapNativeProof "${TRAP_DIR}/fixture.bin")
if(DOOM_ROM)
set(GEN "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED "${GEN}/doom_dispatch.c" "${GEN}/doom_layout.c")
foreach(N RANGE 0 31)
  if(N LESS 10)
    set(N "0${N}")
  endif()
  list(APPEND GENERATED "${GEN}/doom_part${N}.c")
endforeach()
add_custom_command(OUTPUT ${GENERATED}
  COMMAND "${Python3_EXECUTABLE}" "${ROOT}/tools/generate_boot.py" --rom "${DOOM_ROM}"
    --compiler "$<TARGET_FILE:GenesisRecomp>" --output "${GEN}"
  DEPENDS GenesisRecomp "${DOOM_ROM}" "${ROOT}/config/doom_boot.toml"
    "${ROOT}/config/rom_identity.json" "${ROOT}/tools/generate_boot.py" "${ROOT}/tools/rom_tool.py"
  VERBATIM)
genesisrecomp_runner_sources(SOURCES TRACE ON REVERSE_DEBUG OFF)
add_executable(DoomBootProbe ${SOURCES} ${GENERATED} doom_spec.c)
target_include_directories(DoomBootProbe PRIVATE "${RUNNER}" "${RUNNER}/include"
  "${RUNNER}/video" "${RUNNER}/external/ymfm/src" "${RUNNER}/external/superzazu"
  "${RUNNER}/external/clowncommon" "${RECOMP_ROOT}/recompiler/src"
  "${SDLROOT}/include")
target_compile_definitions(DoomBootProbe PRIVATE INCLUDE_GENERATED_CODE=1
  ENABLE_RECOMPILED_CODE=1 OWN_BACKEND=1 GEN_ENABLE_TRACE=1
  DEFAULT_DEBUG_PORT=4490 RECOMP_LAUNCHER=0 GENESIS_HAS_RECOMP_NET=0)
target_compile_options(DoomBootProbe PRIVATE /W3 /utf-8 /bigobj /wd4996)
target_link_libraries(DoomBootProbe PRIVATE SDL2::SDL2main SDL2::SDL2 ws2_32 comdlg32)
genesisrecomp_runner_target(DoomBootProbe)
add_custom_command(TARGET DoomBootProbe POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
  copy_if_different "${SDLROOT}/lib/x64/SDL2.dll" "$<TARGET_FILE_DIR:DoomBootProbe>")
endif()
