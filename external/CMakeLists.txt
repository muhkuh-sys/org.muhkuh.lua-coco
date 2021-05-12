cmake_minimum_required(VERSION 3.7)

PROJECT("external")


# Enable tests (see here for details: http://www.vtk.org/Wiki/CMake/Testing_With_CTest)
ENABLE_TESTING()

#----------------------------------------------------------------------------
#
# Find all packages.
#

# Python is used for some test scripts.
FIND_PACKAGE(PythonInterp 2.7 REQUIRED)


#----------------------------------------------------------------------------
#
# Include the projects in the subdirectories.
#
ADD_SUBDIRECTORY(libusb)
ADD_SUBDIRECTORY(libftdi)

#----------------------------------------------------------------------------

