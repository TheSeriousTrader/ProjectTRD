cmake_minimum_required(VERSION 3.13.4)

# --------------------------------------------------------------------------
# Package and CMake setup

# set the project name
project(trdbox)

# use additional local modules, using contributions from
#  - https://github.com/rpavlik/cmake-modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11)

# find required packages
# find_package(LibTRD)

# DIM does not support CMake packages (yet), so let's hardcode it
# find_package(DIM)

include_directories(/usr/include/liblogicbox)


# --------------------------------------------------------------------------
# get version number from git tags, ignoring the initial v
git_describe_working_tree(GITDESC --tags --match "v\[0-9\]*")
string(SUBSTRING ${GITDESC} 1 -1 VERSION)
message("return: ${GITDESC}")

# if the version is not found (e.g. in a tarball), get if from the directory
if(VERSION STREQUAL "128-NOTFOUND")
  string(REGEX REPLACE ".*/trdbox-v([0-9\.]+)" "\\1" VERSION ${CMAKE_SOURCE_DIR})
endif()

message("TRDbox version: ${VERSION}")

# --------------------------------------------------------------------------
# Packaging

# set(CPACK_PACKAGE_VERSION ${VERSION})
# # set(CPACK_GENERATOR "RPM")
# set(CPACK_GENERATOR "DEB" "TGZ")
# set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
# set(CPACK_PACKAGE_RELEASE 1)
# set(CPACK_PACKAGE_CONTACT "Tom Dietel")
# set(CPACK_PACKAGE_VENDOR "ALICE TRD")
# set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
# # set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
# # set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.ipkg")
# set(CPACK_DEBIAN_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.ipk")
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libdim (=20.26-5), libtrd (=1.2.8)")
# set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE arm)

#
# # set(CPACK_SOURCE_IGNORE_FILES
# #   \\.git/
# #   build/
# #   debian/
# #   ".*~$"
# # )

include(CPack)
# --------------------------------------------------------------------------
# Build instructions

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(trdboxd
  trdboxd.cc
  # command.cc data_acquisition.cc dim_services.cc trigger.cc
  logicbox/su738.cc)
target_link_libraries(trdboxd dim logicbox PocoFoundation boost_date_time usb zmq)

add_executable(subevd subevd.cc trd_subevent_builder.cc)
target_link_libraries(subevd PocoFoundation PocoUtil zmq)


# install the executable
install(TARGETS trdboxd DESTINATION sbin)
install(TARGETS subevd DESTINATION bin)
# install(TARGETS trdce DESTINATION lib)


# --------------------------------------------------------------------------
# enable_testing()
#
# add_test(NAME Errors COMMAND trapcc tests/error.tcs)
# set_tests_properties(Errors PROPERTIES
#   PASS_REGULAR_EXPRESSION "WARNING: this is a warning"
# )
# set_tests_properties(Errors PROPERTIES
#   FAIL_REGULAR_EXPRESSION "this text should not be printed"
# )
