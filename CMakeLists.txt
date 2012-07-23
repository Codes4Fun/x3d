cmake_minimum_required (VERSION 2.6)

project (x3d)
set (x3d_VERSION_MAJOR 0)
set (x3d_VERSION_MINOR 1)

option (USE_HYDRA "Create the Hydra Modules" OFF)
set (SIXENSE_PATH "${PROJECT_SOURCE_DIR}/../sixenseSDK_linux_OSX" CACHE PATH "Sixense SDK for Hydra")

# set the default install to our binary directory
#set (CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/release" CACHE PATH "install prefix" FORCE)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set (CMAKE_INSTALL_PREFIX
    "${PROJECT_BINARY_DIR}/release" CACHE PATH "x3d install prefix" FORCE)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

configure_file (
  "${PROJECT_SOURCE_DIR}/x3dConfig.h.in"
  "${PROJECT_BINARY_DIR}/x3dConfig.h"
  )
include_directories("${PROJECT_BINARY_DIR}")

set (EXTRA_LIBS X11 GL)

include_directories ("${PROJECT_SOURCE_DIR}/vertex")
add_subdirectory (vertex)
set (EXTRA_LIBS ${EXTRA_LIBS} vertex)

include_directories ("${PROJECT_SOURCE_DIR}/xman")
add_subdirectory (xman)
set (EXTRA_LIBS ${EXTRA_LIBS} xman)

if (USE_HYDRA)
  include_directories ("${PROJECT_SOURCE_DIR}/hydra")
  add_subdirectory (hydra)
  set (EXTRA_LIBS ${EXTRA_LIBS} hydra)

  # link in the sixense library
  if (CMAKE_SIZEOF_VOID_P MATCHES "8")
    # 64 bit
    link_directories ("${SIXENSE_PATH}/lib/linux_x64/release")
    set (EXTRA_LIBS ${EXTRA_LIBS} sixense_x64)
    install (FILES "${SIXENSE_PATH}/lib/linux_x64/release/libsixense_x64.so" DESTINATION bin)
  else (CMAKE_SIZEOF_VOID_P MATCHES "8")
    # 32 bit
    link_directories ("${SIXENSE_PATH}/lib/linux/release")
    set (EXTRA_LIBS ${EXTRA_LIBS} sixense)
    install (FILES "${SIXENSE_PATH}/lib/linux/release/libsixense.so" DESTINATION bin)
  endif (CMAKE_SIZEOF_VOID_P MATCHES "8")
  # this is needed for some c++ symbols for some reason
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs")
endif (USE_HYDRA)

add_executable (x3d phasetest.cpp)

target_link_libraries (x3d ${EXTRA_LIBS})

install (TARGETS x3d DESTINATION bin)
install (FILES "${PROJECT_SOURCE_DIR}/run.sh"
               "${PROJECT_SOURCE_DIR}/server.sh"
               "${PROJECT_SOURCE_DIR}/debug.sh"
               "${PROJECT_SOURCE_DIR}/cdebug.sh" DESTINATION bin)

add_test (x3dRuns x3d -display :9)

