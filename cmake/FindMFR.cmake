# - Try to find MFR components
# Once done this will define
#  MFR_FOUND - System has MFR
#  MFR_LIBRARY - The libraries needed to use  MFR
#  MFR_INCLUDE_DIR - The headers needed to use MFR

find_path(MFR_INCLUDE_DIR NAMES mfr_temperature.h PATH_SUFFIXES rdk/iarmmgrs-hal)
find_library(MFR_LIBRARY NAMES RDKMfrLib)

# Set the results
if(MFR_INCLUDE_DIR)
  set(MFR_INCLUDE_DIRS ${MFR_INCLUDE_DIR})
else()
  message(STATUS "Not Found mfr includes")
endif()

if(MFR_LIBRARY)
  set(MFR_FOUND TRUE)
  set(MFR_LIBRARIES ${MFR_LIBRARY})
else()
  set(MFR_FOUND FALSE)
  message(STATUS "Not Found mfr libs")
endif()

# Provide user feedback
if(MFR_FOUND)
  message(STATUS "Found mfr libs @ ${MFR_LIBRARIES}")
  message(STATUS "Found mfr includes @ ${MFR_INCLUDE_DIRS}")
else()
  message(STATUS "Not Found mfr libs @ ${MFR_LIBRARIES}")
  message(STATUS "Not Found mfr includes @ ${MFR_INCLUDE_DIRS}")
endif()

mark_as_advanced(MFR_INCLUDE_DIR MFR_LIBRARY)
