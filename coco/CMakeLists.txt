cmake_minimum_required(VERSION 3.2)

PROJECT("Led_Analyzer")

SET(PROJECT_VERSION 0.0.1)

# Get the VCS version for the jonchki configuration.
INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/version.cmake)

# Filter the jonchki configuration.
CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/coco.xml
               ${CMAKE_BINARY_DIR}/lua${BUILDCFG_LUA_VERSION}-coco-${PROJECT_VERSION}.xml
               @ONLY)

OPTION(BUILDCFG_ONLY_JONCHKI_CFG "Build only the jonchki configuration. This is used for the resolve phase. The default is OFF."
       "OFF")

#----------------------------------------------------------------------------
#
# Build the project.
#

IF((${BUILDCFG_ONLY_JONCHKI_CFG} STREQUAL "OFF"))
	# Enable tests (see here for details: http://www.vtk.org/Wiki/CMake/Testing_With_CTest)
	ENABLE_TESTING()

	# Set some installation folders.
	INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/muhkuh_installation_folders.cmake)

	# Python is used for some test scripts.
	FIND_PACKAGE(PythonInterp 2.7 REQUIRED)

	FIND_PACKAGE(SWIG REQUIRED)
	INCLUDE(${SWIG_USE_FILE})

	# Select the LUA version.
	INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/muhkuh_lua.cmake)

	MESSAGE("EXTERNAL_LIBUSB_ROOT_DIR: ${EXTERNAL_LIBUSB_ROOT_DIR}")
	MESSAGE("EXTERNAL_LIBFTDI_ROOT_DIR: ${EXTERNAL_LIBFTDI_ROOT_DIR}")

	SET(LIBUSB_INCLUDE_DIR "${EXTERNAL_LIBUSB_ROOT_DIR}/include/libusb-1.0")
	SET(LIBUSB_LIBRARIES "${EXTERNAL_LIBUSB_ROOT_DIR}/lib/libusb-1.0.a")

	SET(LIBFTDI_INCLUDE_DIR "${EXTERNAL_LIBFTDI_ROOT_DIR}/include/libftdi1")
	SET(LIBFTDI_LIBRARIES "${EXTERNAL_LIBFTDI_ROOT_DIR}/lib/libftdi1.a")

	MESSAGE("LIBUSB_INCLUDE_DIR: ${LIBUSB_INCLUDE_DIR}")
	MESSAGE("LIBUSB_LIBRARIES: ${LIBUSB_LIBRARIES}")
	MESSAGE("LIBFTDI_INCLUDE_DIR: ${LIBFTDI_INCLUDE_DIR}")
	MESSAGE("LIBFTDI_LIBRARIES: ${LIBFTDI_LIBRARIES}")

	IF(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		SET(ADDITIONAL_LIBRARIES "")
	ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		SET(ADDITIONAL_LIBRARIES -ludev -pthread)
	ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

	SET_SOURCE_FILES_PROPERTIES(led_analyzer.i PROPERTIES CPLUSPLUS OFF)
	SET_SOURCE_FILES_PROPERTIES(led_analyzer.i PROPERTIES SWIG_FLAGS "")

	SWIG_ADD_MODULE(TARGET_led_analyzer lua led_analyzer.i led_analyzer.c i2c_routines.c io_operations.c tcs3472.c)
	SWIG_LINK_LIBRARIES(TARGET_led_analyzer ${LUA_TARGET} "${LIBFTDI_LIBRARIES}" "${LIBUSB_LIBRARIES}" ${ADDITIONAL_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(TARGET_led_analyzer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
	TARGET_INCLUDE_DIRECTORIES(TARGET_led_analyzer PRIVATE "${LIBFTDI_INCLUDE_DIR}")
	TARGET_INCLUDE_DIRECTORIES(TARGET_led_analyzer PRIVATE "${LIBUSB_INCLUDE_DIR}")

	# Set the name of the output file to "led_analyzer".
	SET_TARGET_PROPERTIES(TARGET_led_analyzer PROPERTIES PREFIX "" OUTPUT_NAME "led_analyzer")

	# On mingw link all compiler libraries static.
	IF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
		SET_PROPERTY(TARGET TARGET_led_analyzer PROPERTY LINK_FLAGS "-static-libgcc -static-libstdc++")
	ENDIF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))

	# Install the lua module.
	INSTALL(TARGETS TARGET_led_analyzer DESTINATION ${INSTALL_DIR_LUA_MODULES})

	# Add tests for this module.
	IF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
		# Here are the MinGW specific tests.
		ADD_TEST(NAME romloader_usb_MinGW_DLL_dependencies
		         COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_HOME_DIRECTORY}/cmake/tests/mingw_dll_dependencies.py -u lua5.1 $<TARGET_FILE:TARGET_led_analyzer>)
	ENDIF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))

	#----------------------------------------------------------------------------
	#
	# Build a package for the selected platform.
	#

	SET(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/../../repository/org/muhkuh/lua/coco/${PROJECT_VERSION})
	SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
	SET(CPACK_PACKAGE_FILE_NAME "lua${BUILDCFG_LUA_VERSION}-coco-${PROJECT_VERSION}-${JONCHKI_PLATFORM}")

	SET(CPACK_GENERATOR "TXZ")
	INCLUDE(CPack)

	IF("${JONCHKI_PLATFORM_DIST_ID}" STREQUAL "windows")
		# Package a windows build.

		# NOTE: Do not install the library again, there is an "INSTALL" already.

		CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/install_windows.lua
		               ${CMAKE_CURRENT_BINARY_DIR}/package/install.lua
		               @ONLY)

		INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/package/install.lua
		        DESTINATION .)
	ELSE("${JONCHKI_PLATFORM_DIST_ID}" STREQUAL "windows")
		# Package a linux build.

		# NOTE: Do not install the library again, there is an "INSTALL" already.

		CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/install_linux.lua
		               ${CMAKE_CURRENT_BINARY_DIR}/package/install.lua
		               @ONLY)

		INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/package/install.lua
		        DESTINATION .)
	ENDIF("${JONCHKI_PLATFORM_DIST_ID}" STREQUAL "windows")

	CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/coco.xml
	               ${CPACK_OUTPUT_FILE_PREFIX}/lua${BUILDCFG_LUA_VERSION}-coco-${PROJECT_VERSION}-${JONCHKI_PLATFORM}.xml
	               @ONLY)


	#-----------------------------------------------------------------------------
	#
	# Create a custom target named "pack".
	# It builds the package and creates the hashes.
	#
	ADD_CUSTOM_TARGET(pack
	                  COMMAND ${CMAKE_MAKE_PROGRAM}
	                  COMMAND ${CMAKE_MAKE_PROGRAM} package
	                  COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_HOME_DIRECTORY}/cmake/tools/generate_hash.py ${CPACK_OUTPUT_FILE_PREFIX}/lua${BUILDCFG_LUA_VERSION}-coco-${PROJECT_VERSION}-${JONCHKI_PLATFORM}.xml
	                  COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_HOME_DIRECTORY}/cmake/tools/generate_hash.py ${CPACK_OUTPUT_FILE_PREFIX}/lua${BUILDCFG_LUA_VERSION}-coco-${PROJECT_VERSION}-${JONCHKI_PLATFORM}.tar.xz
	                  COMMENT "Pack the artifact and generate the hashes...")


	#-----------------------------------------------------------------------------
	#
	# Filter the pom.xml file.
	#
	SET(POM_ARTIFACT_ID  "lua${BUILDCFG_LUA_VERSION}-coco")
	SET(POM_GROUP_ID     "org.muhkuh.lua")
	SET(POM_VERSION      "${PROJECT_VERSION}")
	SET(POM_PACKAGING    "tar.xz")
	CONFIGURE_FILE(installer/lua${BUILDCFG_LUA_VERSION}/pom.xml
	               ${CPACK_OUTPUT_FILE_PREFIX}/lua${BUILDCFG_LUA_VERSION}-coco-${PROJECT_VERSION}.pom
	               @ONLY)
ENDIF((${BUILDCFG_ONLY_JONCHKI_CFG} STREQUAL "OFF"))
