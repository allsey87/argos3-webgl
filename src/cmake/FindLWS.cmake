# Find libwebsockets: https://libwebsockets.org/
#
# This module defines these variables:
#
#  LWS_FOUND       - True if the websockets library was found
#  LWS_LIBRARY     - The location of the websockets library
#  LWS_INCLUDE_DIR - The include directory of the websockets library
#
# AUTHOR: Michael Allwright <allsey87@gmail.com>

SET (LWS_FOUND 0)

#
# Find the header file
#
FIND_PATH(LWS_INCLUDE_DIR
  NAMES
  libwebsockets.h
  PATHS
  /usr/include/
  /usr/local/include/
  DOC "Libwebsockets header location"
)

#
# Find the library
#
FIND_LIBRARY(LWS_LIBRARY
  NAMES
  websockets
  PATHS
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64
  DOC "Libwebsockets library location"
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (
  LWS
  FOUND_VAR LWS_FOUND
  REQUIRED_VARS LWS_LIBRARY LWS_INCLUDE_DIR)

