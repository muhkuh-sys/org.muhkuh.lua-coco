cmake_minimum_required(VERSION 2.8)


PROJECT("led_analyzer")


# Swig is required to generate the bindings.
FIND_PACKAGE(SWIG REQUIRED)

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
