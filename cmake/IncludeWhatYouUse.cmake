find_program(IWYU_EXE iwyu)
mark_as_advanced(FORCE IWYU_EXE)
if(IWYU_EXE)
  message(STATUS "include-what-you-use found: ${IWYU_EXE}")
endif()

macro(enable_iwyu)
  if(IWYU_EXE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXE} ${ARGN})
    message(STATUS "include-what-you-use finished setting up.")
  else()
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
        ""
        CACHE STRING "" FORCE)
    message(
      SEND_ERROR "include-what-you-use requested but executable not found.")
  endif()
endmacro()

find_package(Python)

set(IWYU_TOOL_EXE_NAME iwyu_tool)
find_program(IWYU_TOOL_EXE NAMES ${IWYU_TOOL_EXE_NAME} ${IWYU_TOOL_EXE_NAME}.py)
mark_as_advanced(FORCE IWYU_TOOL_EXE)
if(IWYU_TOOL_EXE)
  message(STATUS "${IWYU_TOOL_EXE_NAME} found: ${IWYU_TOOL_EXE}")
endif()

set(FIX_INCLUDE_EXE_NAME fix_include)
find_program(FIX_INCLUDE_EXE NAMES ${FIX_INCLUDE_EXE_NAME}
                                   ${FIX_INCLUDE_EXE_NAME}.py)
mark_as_advanced(FORCE FIX_INCLUDE_EXE)
if(FIX_INCLUDE_EXE)
  message(STATUS "${FIX_INCLUDE_EXE_NAME} found: ${FIX_INCLUDE_EXE}")
endif()

function(enable_iwyu_targets)
  if(IWYU_EXE
     AND Python_FOUND
     AND IWYU_TOOL_EXE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS
        ON
        PARENT_SCOPE)
    add_custom_target(
      iwyu
      COMMAND "${IWYU_TOOL_EXE}" -p "${CMAKE_BINARY_DIR}"
      COMMENT "Running ${IWYU_TOOL_EXE_NAME}"
      VERBATIM)

    if(FIX_INCLUDE_EXE)
      add_custom_target(
        fix-iwyu
        COMMAND "${IWYU_TOOL_EXE}" -p "${CMAKE_BINARY_DIR}" |
                "${FIX_INCLUDE_EXE}" || true
        COMMENT "Running ${IWYU_TOOL_EXE_NAME} (fix)"
        VERBATIM)
    else()
      message(
        SEND_ERROR
          "fix-iwyu target could not be added: ${FIX_INCLUDE_EXE_NAME} not found."
      )
    endif()

  else()
    message(SEND_ERROR "iwyu target could not be added.")
  endif()
endfunction()
