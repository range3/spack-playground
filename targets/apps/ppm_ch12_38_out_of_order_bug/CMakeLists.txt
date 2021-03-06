set(EXE_TARGET_NAME ppm_ch12_38_out_of_order_bug)

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
find_package(fmt REQUIRED)
find_package(TBB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Libpmem REQUIRED IMPORTED_TARGET libpmem)
pkg_check_modules(Libpmemobj REQUIRED IMPORTED_TARGET libpmemobj)
pkg_check_modules(Libpmemobj++ REQUIRED IMPORTED_TARGET libpmemobj++)

target_link_libraries(${EXE_TARGET_NAME} PRIVATE
  cxxopts::cxxopts fmt::fmt
  PkgConfig::Libpmemobj++
  PkgConfig::Libpmemobj
  PkgConfig::Libpmem)

install(TARGETS ${EXE_TARGET_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
