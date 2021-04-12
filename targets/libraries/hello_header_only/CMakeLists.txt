set(THIS_TARGET_NAME hello_header_only)

# List of HPP (header) library files.
set(${THIS_TARGET_NAME}_HDR # cmf: sortable
                            include/hello_header_only/hello_header_only.hpp)

# List of CPP (source) library files.
set(${THIS_TARGET_NAME}_SRC # cmf: sortable
                            "")

set(is_header_only TRUE)
set(inheritance_type INTERFACE)
if(${THIS_TARGET_NAME}_SRC)
  set(is_header_only FALSE)
  set(inheritance_type PUBLIC)
endif()

if(is_header_only)
  add_library(${THIS_TARGET_NAME} INTERFACE)
else()
  add_library(${THIS_TARGET_NAME} ${${PROJECT_NAME_UPPERCASE}_BUILD_TYPE})
endif()
add_library(${PROJECT_NAME}::${THIS_TARGET_NAME} ALIAS ${THIS_TARGET_NAME})

if(NOT is_header_only)
  target_sources(${THIS_TARGET_NAME} PRIVATE ${${THIS_TARGET_NAME}_SRC}
                                             ${${THIS_TARGET_NAME}_HDR})
endif()

target_compile_features(${THIS_TARGET_NAME} ${inheritance_type} cxx_std_20)
target_compile_options(${THIS_TARGET_NAME} ${inheritance_type}
                       ${PROJECT_WARNINGS})

# -std=c++20
if(NOT is_header_only)
  set_target_properties(${THIS_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)

  # -flto
  if(${PROJECT_NAME_UPPERCASE}_INTERPROCEDURAL_OPTIMIZATION)
    set_target_properties(${THIS_TARGET_NAME}
                          PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
  endif()

  # -fPIC
  if(${PROJECT_NAME_UPPERCASE}_BUILD_SHARED_LIBS)
    set_target_properties(${THIS_TARGET_NAME}
                          PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()

  set_target_properties(${THIS_TARGET_NAME}
                        PROPERTIES VERSION ${${PROJECT_NAME}_VERSION})
endif()

set_target_properties(${THIS_TARGET_NAME} PROPERTIES PUBLIC_HEADER
                                                     ${${THIS_TARGET_NAME}_HDR})

target_include_directories(
  ${THIS_TARGET_NAME} ${inheritance_type}
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")

install(
  TARGETS ${THIS_TARGET_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${THIS_TARGET_NAME}"
                COMPONENT dev)

message(STATUS "Created target ${THIS_TARGET_NAME} for export ${PROJECT_NAME}.")
