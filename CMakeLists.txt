cmake_minimum_required(VERSION 3.18)

project(ProntoRandomRaster LANGUAGES CXX)

# --- Dependencies with vcpkg ---
# Find the necessary packages using vcpkg's manifest mode.
find_package(gdal CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# --- Project Definition ---
# Add the 'include' directory for header files.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create the shared library for the GDAL driver from your source files.
add_library(pronto_random_raster SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_dataset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_driver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_band.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_parameters.cpp
)
set_target_properties(pronto_random_raster PROPERTIES
    OUTPUT_NAME "gdal_RANDOM_RASTER"
)

# --- Output Location for built DLL (Temporary during build) ---
# Keep this so Visual Studio puts the DLL in a standard spot after compilation.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY (build output) set to: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}") # Diagnostic


# Link the necessary libraries to your driver.
target_link_libraries(pronto_random_raster PRIVATE GDAL::GDAL nlohmann_json::nlohmann_json)

# --- Output Location for built DLL ---
# Define a specific directory within the build tree for installed GDAL plugins.
set(GDAL_PLUGIN_INSTALL_DIR "${CMAKE_BINARY_DIR}/gdal_plugins")
message(STATUS "GDAL_PLUGIN_INSTALL_DIR (plugin install target) set to: ${GDAL_PLUGIN_INSTALL_DIR}")

# Ensure the plugin directory exists
file(MAKE_DIRECTORY ${GDAL_PLUGIN_INSTALL_DIR})

# Add a POST_BUILD custom command to copy the DLL to the GDAL_PLUGIN_INSTALL_DIR
add_custom_command(TARGET pronto_random_raster POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:pronto_random_raster>
            ${GDAL_PLUGIN_INSTALL_DIR}/$<TARGET_FILE_NAME:pronto_random_raster>
    COMMENT "Copying driver to plugin directory: ${GDAL_PLUGIN_INSTALL_DIR}"
)

# The install() command is for when you run 'cmake --install'
install(TARGETS pronto_random_raster
    DESTINATION ${GDAL_PLUGIN_INSTALL_DIR}
    COMPONENT gdal_driver
)

# --- Python Testing Setup ---
#
# Construct paths using vcpkg-provided variables.
set(VCPKG_TRIPLET_INSTALL_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
message(STATUS "Vcpkg triplet install dir: ${VCPKG_TRIPLET_INSTALL_DIR}")

set(VCPKG_PYTHON_EXECUTABLE "${VCPKG_TRIPLET_INSTALL_DIR}/tools/python3/python.exe")
message(STATUS "Expecting vcpkg Python executable at: ${VCPKG_PYTHON_EXECUTABLE}")

# Helper targets for locating python, pytest, and gdal-installer in the venv
add_executable(python_in_venv IMPORTED)
add_executable(pytest_in_venv IMPORTED)
#add_executable(gdal_installer_in_venv IMPORTED)

if(WIN32)
    set_property(TARGET python_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/Scripts/python.exe)
    set_property(TARGET pytest_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/Scripts/pytest.exe)
    #set_property(TARGET gdal_installer_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/Scripts/gdal-installer.exe)
else()
    set_property(TARGET python_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/bin/python)
    set_property(TARGET pytest_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/bin/pytest)
    #set_property(TARGET gdal_installer_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/bin/gdal-installer)
endif()

# Define a custom target to set up the testing environment.
add_custom_target(setup_test_env
    COMMAND ${VCPKG_PYTHON_EXECUTABLE} -m venv venv
    COMMAND $<TARGET_FILE:python_in_venv> -m pip install --upgrade pip
    COMMAND $<TARGET_FILE:python_in_venv> -m pip install -r ${CMAKE_SOURCE_DIR}/tests/requirements.txt
    # Run the specific Python script for GDAL installation
    COMMAND $<TARGET_FILE:python_in_venv> ${CMAKE_BINARY_DIR}/venv/Lib/site-packages/gdal_installer/install-gdal.py
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating Python virtual environment, installing dependencies, and GDAL"
)

# --- Test Execution Setup ---

# Path to the GDAL DLLs installed by vcpkg for the C++ build.
set(VCPKG_GDAL_BIN_PATH "${VCPKG_TRIPLET_INSTALL_DIR}/bin")

# Construct the full PATH environment variable for the test execution.
# The order is critical: vcpkg path must come first to ensure the correct
# GDAL DLLs are loaded, matching the ones the driver was compiled against.
set(FULL_TEST_PATH "${GDAL_PLUGIN_INSTALL_DIR};${VCPKG_GDAL_BIN_PATH};$ENV{PATH}")

# Define a custom target to run the pytest tests.
add_custom_target(run_tests
    COMMAND ${CMAKE_COMMAND} -E env
            "VCPKG_GDAL_BIN_DIR=${VCPKG_GDAL_BIN_PATH}"
            "GDAL_DRIVER_PATH=${GDAL_PLUGIN_INSTALL_DIR}"
            "PATH=${FULL_TEST_PATH}"
            $<TARGET_FILE:pytest_in_venv>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
    DEPENDS setup_test_env pronto_random_raster
    COMMENT "Running pytest with GDAL_DRIVER_PATH set"
)