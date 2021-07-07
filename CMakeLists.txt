cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(
  spack_playground
  VERSION 0.1.0
  LANGUAGES CXX ASM)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
include(GNUInstallDirs)
include(cmake/CompilerWarnings.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Prevent in-source builds
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# Determine whether this is a root project or included by other projects
set(${PROJECT_NAME_UPPERCASE}_ROOT_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(${PROJECT_NAME_UPPERCASE}_ROOT_PROJECT ON)
endif()

# The project options
include(CMakeDependentOption)
option(${PROJECT_NAME_UPPERCASE}_BUILD_TESTS "Build tests"
       ${${PROJECT_NAME_UPPERCASE}_ROOT_PROJECT})
option(${PROJECT_NAME_UPPERCASE}_FORMAT
       "Enable targets: format/check-format/fix-format"
       ${${PROJECT_NAME_UPPERCASE}_ROOT_PROJECT})
option(${PROJECT_NAME_UPPERCASE}_CLANG_TIDY "Enable clang-tidy" ON)
option(${PROJECT_NAME_UPPERCASE}_IWYU "Enable iwyu(include-what-you-use)" ON)
option(${PROJECT_NAME_UPPERCASE}_BUILD_SHARED_LIBS "build as a shared library"
       ${BUILD_SHARED_LIBS})
option(${PROJECT_NAME_UPPERCASE}_INTERPROCEDURAL_OPTIMIZATION
       "Enable interprocedural optimization (-flto)" OFF)
option(${PROJECT_NAME_UPPERCASE}_ENABLE_RPATH
       "Enable RPATH support for installed binaries and libraries" OFF)

# check capability of interprocedual optimization
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION
   OR ${PROJECT_NAME_UPPERCASE}_INTERPROCEDURAL_OPTIMIZATION)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result)
  if(result)
    message(STATUS "Interprocedural optimization is enabled.")
  else()
    message(FATAL_ERROR "Interprocedural optimization is not supported.")
  endif()
endif()

# Control where libraries and executables are placed during the build for convenience.
if(${PROJECT_NAME_UPPERCASE}_ROOT_PROJECT)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif()

# Library build type
set(${PROJECT_NAME_UPPERCASE}_BUILD_TYPE STATIC)
if(${PROJECT_NAME_UPPERCASE}_BUILD_SHARED_LIBS)
  set(${PROJECT_NAME_UPPERCASE}_BUILD_TYPE SHARED)
endif()

# Enable RPATH support for installed binaries and libraries
if(${PROJECT_NAME_UPPERCASE}_ENABLE_RPATH)
  include(cmake/AddInstallRPATHSupport.cmake)
  add_install_rpath_support(
    BIN_DIRS "${CMAKE_INSTALL_FULL_BINDIR}"
    LIB_DIRS "${CMAKE_INSTALL_FULL_LIBDIR}"
    INSTALL_NAME_DIR "${CMAKE_INSTALL_FULL_LIBDIR}"
    USE_LINK_PATH)
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  # This dreadful mess is to communicate to clang-tidy the C++ system includes.
  # It seems that CMake doesn't support using its own compile_commands.json
  # database, and that clang-tidy doesn't pick up non-default system headers.
  execute_process(
    COMMAND
      bash -c
      "${CMAKE_CXX_COMPILER} -x c++ -Wp,-v /dev/null 2>&1 > /dev/null | grep '^ /' | grep -w 'c++'"
    OUTPUT_VARIABLE COMPILER_HEADERS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX REPLACE "[ \n\t]+" ";" INCLUDE_COMPILER_HEADERS
                       ${COMPILER_HEADERS})

  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${INCLUDE_COMPILER_HEADERS})
endif()

# clang-tidy
if(${PROJECT_NAME_UPPERCASE}_CLANG_TIDY)
  include(cmake/ClangTidy.cmake)
  # enable_clang_tidy()
  enable_clang_tidy_targets()
endif()

# include-what-you-use
if(${PROJECT_NAME_UPPERCASE}_IWYU)
  include(cmake/IncludeWhatYouUse.cmake)
  # enable_iwyu()
  enable_iwyu_targets()
endif()

# Enable targets: format/check-format/fix-format by Format.cmake
if(${PROJECT_NAME_UPPERCASE}_FORMAT)
  include(cmake/CPM.cmake)
  CPMAddPackage(
    NAME Format.cmake
    VERSION 1.7.0
    GITHUB_REPOSITORY TheLartians/Format.cmake
    OPTIONS "FORMAT_SKIP_CMAKE NO" "CMAKE_FORMAT_EXCLUDE (^|/)cmake")
endif()

# Encourage user to specify a build type (e.g. Release, Debug, etc.), otherwise
# set it to Release.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

# dependencies Conan package manager: support both generators cmake and
# cmake_multi
if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
  conan_basic_setup(TARGETS)
elseif(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
endif()

# depenencies

# source files
add_subdirectory(targets)

# Enable testing
if(${PROJECT_NAME_UPPERCASE}_BUILD_TESTS)
  add_subdirectory(tests)
endif()

#[[
Target ${PROJECT_NAME}_version
Install ${PROJECT_NAME_LOWERCASE}/version.h]]
include(cmake/Version.cmake)

# Install ${PROJECT_NAME}Config.cmake for find_package(<PROJECT_NAME>)
# http://robotology.github.io/ycm/gh-pages/v0.12/module/InstallBasicPackageFiles.html
include(cmake/InstallBasicPackageFiles.cmake)
install_basic_package_files(
  ${PROJECT_NAME}
  VERSION ${${PROJECT_NAME}_VERSION}
  COMPATIBILITY SameMajorVersion
  VARS_PREFIX ${PROJECT_NAME_UPPERCASE}
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)

# Add the uninstall target
include(cmake/AddUninstallTarget.cmake)
