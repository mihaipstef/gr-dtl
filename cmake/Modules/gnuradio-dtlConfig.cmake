find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_DTL gnuradio-dtl)

FIND_PATH(
    GR_DTL_INCLUDE_DIRS
    NAMES gnuradio/dtl/api.h
    HINTS $ENV{DTL_DIR}/include
        ${PC_DTL_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_DTL_LIBRARIES
    NAMES gnuradio-dtl
    HINTS $ENV{DTL_DIR}/lib
        ${PC_DTL_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-dtlTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_DTL DEFAULT_MSG GR_DTL_LIBRARIES GR_DTL_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_DTL_LIBRARIES GR_DTL_INCLUDE_DIRS)
