cmake_minimum_required(VERSION 3.7)


PROJECT("led_analyzer")

# Get the VCS version for the jonchki configuration.
INCLUDE(cmake/version.cmake)
INCLUDE(cmake/platform.cmake)
MESSAGE("Building for: ${JONCHKI_PLATFORM}")

# Find the Doxygen package
FIND_PACKAGE(Doxygen)

#----------------------------------------------------------------------------
#
# Include the projects in the subdirectories.

add_subdirectory(coco)


#----------------------------------------------------------------------------
#
# Create a doxygen documentation 
#
#IF(DOXYGEN_FOUND)
#	CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
#	ADD_CUSTOM_TARGET(doc ALL
#	                  ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
#	                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#	                  COMMENT "Generating API documentation with Doxygen" VERBATIM)
#ENDIF(DOXYGEN_FOUND)
