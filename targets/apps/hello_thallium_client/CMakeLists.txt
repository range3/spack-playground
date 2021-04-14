set(EXE_TARGET_NAME hello_thallium_client)

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
find_package(mercury REQUIRED)
find_package(thallium REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARGO REQUIRED IMPORTED_TARGET margo)
pkg_check_modules(ABT REQUIRED IMPORTED_TARGET argobots)
# pkg_check_modules(ABTIO REQUIRED IMPORTED_TARGET abt-io)
# pkg_check_modules(SSG REQUIRED IMPORTED_TARGET ssg)

target_link_libraries(${EXE_TARGET_NAME}
                      PRIVATE cxxopts::cxxopts thallium)

install(TARGETS ${EXE_TARGET_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
