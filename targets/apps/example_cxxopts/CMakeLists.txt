set(EXE_TARGET_NAME example_cxxopts)

set(${EXE_TARGET_NAME}_SRC # cmf: sortable
                           main.cpp)

add_executable(${EXE_TARGET_NAME})

target_sources(${EXE_TARGET_NAME} PRIVATE ${${EXE_TARGET_NAME}_SRC})

target_compile_features(${EXE_TARGET_NAME} PUBLIC cxx_std_20)
set_target_properties(${EXE_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)

if(${PROJECT_NAME_UPPERCASE}_INTERPROCEDURAL_OPTIMIZATION)
  set_target_properties(${EXE_TARGET_NAME}
                        PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

find_package(cxxopts REQUIRED)

target_link_libraries(${EXE_TARGET_NAME}
                      PRIVATE cxxopts::cxxopts)

install(TARGETS ${EXE_TARGET_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
