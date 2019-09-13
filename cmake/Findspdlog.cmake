# Standard FIND_PACKAGE module for spdlog, sets the following variables:
#   - LIBSPDLOG_FOUND
#   - LIBSPDLOG_INCLUDE_DIRS (only if LIBSPDLOG_FOUND)

# Try to find the header
FIND_PATH(LIBSPDLOG_INCLUDE_DIR NAMES "spdlog/spdlog.h")

# Handle the QUIETLY/REQUIRED arguments, set LIBSPDLOG_FOUND if all variables are
# found
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBSPDLOG
								  REQUIRED_VARS
								  LIBSPDLOG_INCLUDE_DIR)

# Hide internal variables
MARK_AS_ADVANCED(LIBSPDLOG_INCLUDE_DIR)

# Set standard variables
IF(LIBSPDLOG_FOUND)
	SET(LIBSPDLOG_INCLUDE_DIRS "${LIBSPDLOG_INCLUDE_DIR}")
ENDIF()
