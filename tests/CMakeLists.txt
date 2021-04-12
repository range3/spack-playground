set(TEST_TARGET_NAME cppp_test)

set(${TEST_TARGET_NAME}_SRC # cmf: sortable
                            main.cpp test_hellolib.cpp)

add_executable(${TEST_TARGET_NAME})
target_sources(${TEST_TARGET_NAME} PRIVATE ${${TEST_TARGET_NAME}_SRC})
target_link_libraries(
  ${TEST_TARGET_NAME}
  PRIVATE CONAN_PKG::doctest ${PROJECT_NAME}::hello_header_only
          ${PROJECT_NAME}::hellolib)

target_compile_features(${TEST_TARGET_NAME} PUBLIC cxx_std_20)
set_target_properties(${TEST_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
if(${PROJECT_NAME_UPPERCASE}_INTERPROCEDURAL_OPTIMIZATION)
  set_target_properties(${TEST_TARGET_NAME}
                        PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

include(${CONAN_LIB_DIRS_DOCTEST}/cmake/doctest.cmake)
doctest_discover_tests(${TEST_TARGET_NAME})
