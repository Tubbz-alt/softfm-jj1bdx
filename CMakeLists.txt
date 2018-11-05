# CMake definitions for SoftFM

cmake_minimum_required(VERSION 3.0.2)
project(SoftFM)

# Workaround for the compilation warning
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(Threads)
find_package(PkgConfig)

# Find RTL-SDR library (for linking).
pkg_check_modules(PKG_RTLSDR librtlsdr)
find_path(RTLSDR_INCLUDE_DIR rtl-sdr.h
          HINT ${PKG_RTLSDR_INCLUDE_DIRS})
find_library(RTLSDR_LIBRARY librtlsdr.a
             HINT ${PKG_RTLSDR_LIBRARY_DIRS})

# Find libusb
# See https://github.com/texane/stlink/blob/master/cmake/modules/FindLibUSB.cmake
pkg_check_modules(PKG_LIBUSB libusb-1.0)
if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
	find_path(LIBUSB_INCLUDE_DIR libusb.h
        HINTS /usr/include ${PKG_LIBUSB_INCLUDE_DIRS}
    )
else ()
	find_path(LIBUSB_INCLUDE_DIR libusb.h
        HINTS /usr /usr/local /opt ${PKG_LIBUSB_INCLUDE_DIRS}
        PATH_SUFFIXES libusb-1.0
	)
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(LIBUSB_NAME usb)
else ()
    set(LIBUSB_NAME usb-1.0)
endif()
find_library(LIBUSB_LIBRARY ${LIBUSB_NAME}
             HINTS /usr /usr/local /opt ${PKG_LIBUSB_LIBRARY_DIRS})

if(RTLSDR_INCLUDE_DIR AND RTLSDR_LIBRARY)
    message(STATUS "Found librtlsdr: ${RTLSDR_INCLUDE_DIR}, ${RTLSDR_LIBRARY}")
else()
    message(WARNING "Can not find Osmocom RTL-SDR library")
    message("Try again with environment variable PKG_CONFIG_PATH")
    message("or with -DRTLSDR_INCLUDE_DIR=/path/rtlsdr/include")
    message("        -DRTLSDR_LIBRARY=/path/rtlsdr/lib/librtlsdr.a")
endif()

set(RTLSDR_INCLUDE_DIRS ${RTLSDR_INCLUDE_DIR} ${LIBUSB_INCLUDE_DIR})
set(RTLSDR_LIBRARIES    ${RTLSDR_LIBRARY} ${LIBUSB_LIBRARY})

# Compiler flags.
set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -O3 -ffast-math -ftree-vectorize ${EXTRA_FLAGS}")

set(sfmbase_SOURCES
    sfmbase/RtlSdrSource.cpp
    sfmbase/Filter.cpp
    sfmbase/FmDecode.cpp
    sfmbase/AudioOutput.cpp
)

set(sfmbase_HEADERS
	include/AudioOutput.h
	include/Filter.h
	include/FmDecode.h
	include/RtlSdrSource.h
	include/SoftFM.h
)

set(sfmbase_SOURCES
	${sfmbase_SOURCES}
	${sfmbase_HEADERS}
)

add_library(sfmbase STATIC
	${sfmbase_SOURCES}
)

add_executable(softfm
	main.cpp
)

include_directories(
	${CMAKE_SOURCE_DIR}/include
    ${RTLSDR_INCLUDE_DIRS}
    ${EXTRA_INCLUDES}
)

target_link_libraries(softfm
	sfmbase
    ${CMAKE_THREAD_LIBS_INIT}
    ${RTLSDR_LIBRARIES}
    ${EXTRA_LIBS}
)

install(TARGETS softfm DESTINATION bin)
install(TARGETS sfmbase DESTINATION lib)

