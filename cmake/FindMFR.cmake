# - Try to find MFR components
# Once done this will define
#  MFR_FOUND - System has MFR
#  MFR_LIBRARIES - The libraries needed to use  MFR
#  MFR_INCLUDE_DIRS - The headers needed to use MFR

find_package(PkgConfig)

find_library(MFR_LIBRARIES NAMES RDKMfrLib)
find_path(MFR_INCLUDE_DIRS NAMES mfr_temperature.h PATH_SUFFIXES /usr/include/mfr/include)

set(MFR_INCLUDE_DIRS ${MFR_INCLUDE_DIRS})
set(MFR_INCLUDE_DIRS ${MFR_INCLUDE_DIRS} CACHE PATH "Path to MFR include")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MFR DEFAULT_MSG MFR_INCLUDE_DIRS MFR_LIBRARIES)

mark_as_advanced(
    MFR_FOUND
    MFR_INCLUDE_DIRS
    MFR_LIBRARIES)

