cmake_minimum_required(VERSION 3.16)

project(
  tig-cli
  VERSION 0.1.0
  DESCRIPTION "Command line tool for TiG (Timeline in Git) repositories"
  LANGUAGES C
)

option(TIG_CLI_BUILD_TESTS "Build tests" ON)
set(TIG_CLI_VERSION_SUFFIX "" CACHE STRING "Appended to the version, e.g. +42.1a2b3c4 for CI builds")
set(TIG_CLI_VERSION "${PROJECT_VERSION}${TIG_CLI_VERSION_SUFFIX}")

if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

# A pinned libgit2, built into libtig, as in tig-agent. The CLI never talks to a network itself (clone and push are
# left to the git command), so libgit2 needs no HTTPS or SSH and the binary depends on nothing beyond libc.
include(FetchContent)
FetchContent_Declare(
  libgit2
  URL https://github.com/libgit2/libgit2/archive/refs/tags/v1.9.7.tar.gz
  URL_HASH SHA256=1a4fbe7589e814777ae76b64734ad80f4ecad22cd33a22682a2aaea4ae5375e7
)
FetchContent_GetProperties(libgit2)
if(NOT libgit2_POPULATED)
  FetchContent_Populate(libgit2)
endif()

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE)
set(USE_HTTPS OFF CACHE STRING "" FORCE)
set(USE_SSH OFF CACHE STRING "" FORCE)
set(USE_SHA256 Builtin CACHE STRING "" FORCE)
set(USE_GSSAPI OFF CACHE BOOL "" FORCE)
set(USE_ICONV OFF CACHE BOOL "" FORCE)
set(USE_BUNDLED_ZLIB ON CACHE STRING "" FORCE)
set(REGEX_BACKEND builtin CACHE STRING "" FORCE)

set(TIG_LIBGIT2_SOURCE_DIR "${libgit2_SOURCE_DIR}" CACHE PATH "" FORCE)
set(TIG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TIG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/tig-lib)

add_library(
  tig-cli-core STATIC
  src/json.c
  src/process.c
  src/signing.c
  src/time.c
  src/cmd_init.c
  src/cmd_log.c
  src/cmd_state.c
)
target_include_directories(tig-cli-core PUBLIC src)
target_link_libraries(tig-cli-core PUBLIC tig::tig)
target_compile_definitions(tig-cli-core PUBLIC TIG_CLI_VERSION="${TIG_CLI_VERSION}")

add_executable(tig-cli src/main.c)
set_target_properties(tig-cli PROPERTIES OUTPUT_NAME tig)
target_link_libraries(tig-cli PRIVATE tig-cli-core)

foreach(_target tig-cli-core tig-cli)
  target_compile_features(${_target} PRIVATE c_std_99)
  target_compile_definitions(${_target} PRIVATE _DEFAULT_SOURCE)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${_target} PRIVATE -Wall -Wextra)
  endif()
endforeach()

if(TIG_CLI_BUILD_TESTS)
  enable_testing()

  add_executable(tig-cli-unit-test tests/unit_test.c)
  target_link_libraries(tig-cli-unit-test PRIVATE tig-cli-core)
  target_compile_definitions(tig-cli-unit-test PRIVATE _DEFAULT_SOURCE)
  add_test(NAME tig-cli-unit-test COMMAND tig-cli-unit-test)

  add_test(NAME tig-cli-end-to-end COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/end_to_end.sh" "$<TARGET_FILE:tig-cli>")
endif()

include(GNUInstallDirs)
install(TARGETS tig-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT cli)
