# find_package(PkgConfig)
# pkg_check_modules(PC_Libpmemkv QUIET libpmemkv)

find_path(Libpmemkv_INCLUDE_DIR
  NAMES libpmemkv.h
  # PATHS ${PC_Libpmemkv_INCLUDE_DIRS}
)
find_library(Libpmemkv_LIBRARY
  NAMES pmemkv
  # PATHS ${PC_Libpmemkv_LIBRARY_DIRS}
)
# set(Libpmemkv_VERSION ${PC_Libpmemkv_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libpmemkv
  FOUND_VAR Libpmemkv_FOUND
  REQUIRED_VARS
    Libpmemkv_LIBRARY
    Libpmemkv_INCLUDE_DIR
  # VERSION_VAR Libpmemkv_VERSION
)

if(Libpmemkv_FOUND)
  set(Libpmemkv_LIBRARIES "${Libpmemkv_LIBRARY}")
  set(Libpmemkv_INCLUDE_DIRS "${Libpmemkv_INCLUDE_DIR}")
  # set(Libpmemkv_DEFINITIONS "${PC_Libpmemkv_CFLAGS_OTHER}")
endif()

if(Libpmemkv_FOUND AND NOT TARGET Libpmemkv::Libpmemkv)
  add_library(Libpmemkv::Libpmemkv UNKNOWN IMPORTED)
  set_target_properties(Libpmemkv::Libpmemkv PROPERTIES
    IMPORTED_LOCATION "${Libpmemkv_LIBRARY}"
    # INTERFACE_COMPILE_OPTIONS "${PC_Libpmemkv_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${Foo_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  Libpmemkv_INCLUDE_DIR
  Libpmemkv_LIBRARY
)
