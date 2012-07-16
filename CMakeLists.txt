cmake_minimum_required (VERSION 2.6)

project (x3d)
set (x3d_VERSION_MAJOR 0)
set (x3d_VERSION_MINOR 1)

option (USE_HYDRA "Create the Hydra Modules" OFF)
set (SIXENSE_PATH "${PROJECT_SOURCE_DIR}/sixenseSDK_linux_OSX" CACHE PATH "Sixense SDK for Hydra")

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

add_library (vertex Matrix.cpp Vector.cpp)
set (EXTRA_LIBS ${EXTRA_LIBS} vertex)

add_library (x3dserver XWindow.cpp)
set (EXTRA_LIBS ${EXTRA_LIBS} x3dserver)

if (USE_HYDRA)
  include_directories ("${SIXENSE_PATH}/include")
  if (CMAKE_SIZEOF_VOID_P MATCHES "8")
    # 64 bit
    link_directories ("${SIXENSE_PATH}/lib/linux_x64/release")
    set (EXTRA_LIBS ${EXTRA_LIBS} sixense_x64)
  else (CMAKE_SIZEOF_VOID_P MATCHES "8")
    # 32 bit
    link_directories ("${SIXENSE_PATH}/lib/linux/release")
    set (EXTRA_LIBS ${EXTRA_LIBS} sixense)
  endif (CMAKE_SIZEOF_VOID_P MATCHES "8")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs")
  add_library (hydra Hydra.cpp)
  set (EXTRA_LIBS ${EXTRA_LIBS} hydra)
endif (USE_HYDRA)

add_executable (x3d phasetest.cpp)

target_link_libraries (x3d ${EXTRA_LIBS})

install (TARGETS x3d DESTINATION bin)

add_test (x3dRuns x3d -display :9)

