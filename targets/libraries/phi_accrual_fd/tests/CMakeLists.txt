set(TEST_TARGET_NAME test_${THIS_TARGET_NAME})

set(${TEST_TARGET_NAME}_SRC # cmf: sortable
                            main.cpp)

add_executable(${TEST_TARGET_NAME})
target_sources(${TEST_TARGET_NAME} PRIVATE ${${TEST_TARGET_NAME}_SRC})
target_compile_features(${TEST_TARGET_NAME} PUBLIC cxx_std_20)
set_target_properties(${EXE_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
if(${PROJECT_NAME_UPPERCASE}_INTERPROCEDURAL_OPTIMIZATION)
  set_target_properties(${TEST_TARGET_NAME}
                        PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

find_package(doctest REQUIRED)
find_package(fmt REQUIRED)

target_link_libraries(
  ${TEST_TARGET_NAME} PRIVATE doctest::doctest fmt::fmt
                              ${PROJECT_NAME}::${THIS_TARGET_NAME})

include(CTest)
enable_testing()
include("${doctest_DIR}/doctest.cmake")
doctest_discover_tests(${TEST_TARGET_NAME} ADD_LABELS ${TEST_TARGET_NAME})
