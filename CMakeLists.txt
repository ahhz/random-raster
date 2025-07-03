cmake_minimum_required(VERSION 3.18)

project(random_raster LANGUAGES CXX)

# --- Dependencies ---
find_package(gdal CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# --- Find Python Interpreter ---
# The vcpkg toolchain can interfere with find_package(Python3). We will locate the
# interpreter manually with a fallback for non-vcpkg environments.
if(DEFINED ENV{VCPKG_TOOLCHAIN_FILE} OR DEFINED VCPKG_TOOLCHAIN_FILE)
    message(STATUS "Vcpkg environment detected. Locating Python interpreter directly.")
    # VCPKG_INSTALLED_DIR is a variable provided by the vcpkg toolchain file.
    if(WIN32)
        set(PYTHON_INTERPRETER "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/python3/python.exe")
    else()
        set(PYTHON_INTERPRETER "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/python3/bin/python3")
    endif()
else()
    message(STATUS "Standard environment detected. Searching for Python interpreter.")
    find_program(PYTHON_INTERPRETER NAMES python3 python)
endif()

# Verify that the Python interpreter was found before proceeding.
if(NOT PYTHON_INTERPRETER)
    message(FATAL_ERROR "Could not find a Python interpreter. Please ensure Python is installed and in the system's PATH, or that the vcpkg python3 port is installed.")
else()
    message(STATUS "Found Python interpreter: ${PYTHON_INTERPRETER}")
endif()


# --- Project Definition ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(gdal_RANDOM_RASTER SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_dataset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_driver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_band.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_parameters.cpp
)

# --- Add Headers to Project ---
target_sources(gdal_RANDOM_RASTER PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_raster_band.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_raster_dataset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_raster_driver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_raster_parameters.h
)

target_link_libraries(gdal_RANDOM_RASTER PRIVATE GDAL::GDAL nlohmann_json::nlohmann_json)

# --- Output Location for built DLL ---
set(GDAL_PLUGIN_INSTALL_DIR "${CMAKE_BINARY_DIR}/gdal_plugins")
file(MAKE_DIRECTORY ${GDAL_PLUGIN_INSTALL_DIR})
message(STATUS "GDAL_PLUGIN_INSTALL_DIR (plugin install target) set to: ${GDAL_PLUGIN_INSTALL_DIR}")

add_custom_command(TARGET gdal_RANDOM_RASTER POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:gdal_RANDOM_RASTER>
            ${GDAL_PLUGIN_INSTALL_DIR}/$<TARGET_FILE_NAME:gdal_RANDOM_RASTER>
    COMMENT "Copying driver to plugin directory: ${GDAL_PLUGIN_INSTALL_DIR}"
)

# --- Python Testing Setup ---
add_executable(python_in_venv IMPORTED)
add_executable(pytest_in_venv IMPORTED)

if(WIN32)
    set_property(TARGET python_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/Scripts/python.exe)
    set_property(TARGET pytest_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/Scripts/pytest.exe)
else()
    set_property(TARGET python_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/bin/python)
    set_property(TARGET pytest_in_venv PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/venv/bin/pytest)
endif()

# --- Python Test Target ---
# This  target sets up the Python virtual environment and executes the tests
add_custom_target(test
    # --- Part 1: Setup Commands ---
    COMMAND "${PYTHON_INTERPRETER}" -m venv venv
    COMMAND $<TARGET_FILE:python_in_venv> -m pip install --upgrade pip
    COMMAND $<TARGET_FILE:python_in_venv> -m pip install -r ${CMAKE_SOURCE_DIR}/tests/requirements.txt
    COMMAND $<TARGET_FILE:python_in_venv> ${CMAKE_BINARY_DIR}/venv/Lib/site-packages/gdal_installer/install-gdal.py

    # --- Part 2: Test Execution Command ---
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR}/tests
            ${CMAKE_COMMAND} -E env
                "PLUGIN_DEP_DIR=$<TARGET_FILE_DIR:GDAL::GDAL>"
                "GDAL_DRIVER_PATH=${GDAL_PLUGIN_INSTALL_DIR}"
                "PATH=${GDAL_PLUGIN_INSTALL_DIR};$<TARGET_FILE_DIR:GDAL::GDAL>;$ENV{PATH}"
                $<TARGET_FILE:pytest_in_venv>

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS gdal_RANDOM_RASTER
    COMMENT "Setting up Python environment and running tests..."
)

# --- Utility project to show test files in the IDE ---
add_custom_target(tests_project)
target_sources(tests_project PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/conftest.py
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_driver_presence.py
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/requirements.txt
)