find_program(CLANG_TIDY_EXE clang-tidy)
mark_as_advanced(FORCE CLANG_TIDY_EXE)
if(CLANG_TIDY_EXE)
  message(STATUS "Clang-Tidy found: ${CLANG_TIDY_EXE}")
endif()

macro(enable_clang_tidy)
  if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE} ${ARGN})
    message(STATUS "Clang-Tidy finished setting up.")
  else()
    set(CMAKE_CXX_CLANG_TIDY
        ""
        CACHE STRING "" FORCE)
    message(SEND_ERROR "Clang-Tidy requested but executable not found.")
  endif()
endmacro()

find_package(Python)

set(RUN_CLANG_TIDY_EXE_NAME run-clang-tidy)
find_program(RUN_CLANG_TIDY_EXE NAMES ${RUN_CLANG_TIDY_EXE_NAME}
                                      ${RUN_CLANG_TIDY_EXE_NAME}.py)
mark_as_advanced(FORCE RUN_CLANG_TIDY_EXE_NAME)
if(RUN_CLANG_TIDY_EXE)
  message(STATUS "${RUN_CLANG_TIDY_EXE_NAME} found: ${RUN_CLANG_TIDY_EXE}")
endif()

function(enable_clang_tidy_targets)
  if(CLANG_TIDY_EXE AND RUN_CLANG_TIDY_EXE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS
        ON
        PARENT_SCOPE)
    add_custom_target(
      clang-tidy
      COMMAND "${RUN_CLANG_TIDY_EXE}" -p "${CMAKE_BINARY_DIR}" -header-filter
              "${CMAKE_SOURCE_DIR}/.*"
      COMMENT "Running ${RUN_CLANG_TIDY_EXE_NAME}"
      VERBATIM)
    add_custom_target(
      fix-clang-tidy
      COMMAND "${RUN_CLANG_TIDY_EXE}" -p "${CMAKE_BINARY_DIR}" -header-filter
              "${CMAKE_SOURCE_DIR}/.*" -fix
      COMMENT "Running ${RUN_CLANG_TIDY_EXE_NAME}"
      VERBATIM)
  else()
    message(SEND_ERROR "clang-tidy target could not be added.")
  endif()
endfunction()
