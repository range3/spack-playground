set(EXE_TARGET_NAME example_cxxoptsj_subcmd)

set(${EXE_TARGET_NAME}_SRC # cmf: sortable
                           main.cpp
                           )

add_executable(${EXE_TARGET_NAME})

target_sources(${EXE_TARGET_NAME} PRIVATE ${${EXE_TARGET_NAME}_SRC})

target_compile_features(${EXE_TARGET_NAME} PUBLIC cxx_std_20)
set_target_properties(${EXE_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)

if(${PROJECT_NAME_UPPERCASE}_INTERPROCEDURAL_OPTIMIZATION)
  set_target_properties(${EXE_TARGET_NAME}
                        PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

find_package(Threads REQUIRED)
find_package(cxxopts REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Libpmemobj++ REQUIRED IMPORTED_TARGET libpmemobj++)

target_link_libraries(${EXE_TARGET_NAME} PRIVATE
  Threads::Threads
  cxxopts::cxxopts
  fmt::fmt
  nlohmann_json::nlohmann_json
  ${PROJECT_NAME}::pretty_bytes
  ${PROJECT_NAME}::gen_random_string
  ${PROJECT_NAME}::elapsedtime
  ${PROJECT_NAME}::psync)

install(TARGETS ${EXE_TARGET_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
