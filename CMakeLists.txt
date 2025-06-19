cmake_minimum_required(VERSION 3.20)

# This CMake file now works "out of the box" in conjunction with vcpkg in manifest mode, along with the vcpkg.json file
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON) 
project(pronto_random_raster)

################################################################
# Configure Project Properties
#
if(MSVC)
    set(PRONTO_CXX_STANDARD 23) # /std:c++latest for msvc
else()
    set(PRONTO_CXX_STANDARD 20)
endif()

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

################################################################
# Define Source Files
#

# 1. Define files for the GDAL DRIVER (SHARED LIBRARY)
set(pronto_driver_files
    ${PROJECT_INCLUDE_DIR}/pronto/raster/block_generator_interface.h
    ${PROJECT_INCLUDE_DIR}/pronto/raster/random_block_generator.h
    ${PROJECT_INCLUDE_DIR}/pronto/raster/random_raster_dataset.h
    ${PROJECT_INCLUDE_DIR}/pronto/raster/random_raster_driver.h
    ${PROJECT_INCLUDE_DIR}/pronto/raster/random_raster_band.h
    ${PROJECT_INCLUDE_DIR}/pronto/raster/random_raster_parameters.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_dataset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_driver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_band.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_parameters.cpp
)

# 2. Define files for the TEST EXECUTABLE
set(pronto_app_files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

################################################################
# Find Dependencies
#
find_package(GDAL CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

################################################################
# Target 1: The GDAL Driver as a SHARED LIBRARY
#
add_library(gdal_RANDOM_RASTER SHARED ${pronto_driver_files})

# Set properties for the driver library
set_target_properties(gdal_RANDOM_RASTER PROPERTIES
    CXX_STANDARD ${PRONTO_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    # Set the output name to be clean, without prefixes like "lib" on Linux
    PREFIX ""
)

# Public include for any code consuming this library (like our test app)
target_include_directories(gdal_RANDOM_RASTER PUBLIC ${PROJECT_INCLUDE_DIR})

# Link the driver library against its dependencies
target_link_libraries(gdal_RANDOM_RASTER PUBLIC GDAL::GDAL)
target_link_libraries(gdal_RANDOM_RASTER PRIVATE nlohmann_json::nlohmann_json)


################################################################
# Target 2: The Test Executable
#
add_executable(random_raster_app ${pronto_app_files})

# Set properties for the test executable
set_target_properties(random_raster_app PROPERTIES CXX_STANDARD ${PRONTO_CXX_STANDARD})

# Link the test app against GDAL to use its functions
target_link_libraries(random_raster_app PRIVATE GDAL::GDAL)
# ADD THIS LINE: Link the executable to your custom driver library
#target_link_libraries(random_raster_app PRIVATE gdal_RANDOM_RASTER)
set(RANDOM_RASTER_DLL_DIR "$<TARGET_FILE_DIR:gdal_RANDOM_RASTER>")
target_compile_definitions(random_raster_app PRIVATE RANDOM_RASTER_DRIVER_PATH="${RANDOM_RASTER_DLL_DIR}")

if(WIN32)
    # On Windows, use cmd /c to set the environment variable and then run the command
    # Use && to chain commands, ensuring the env var is set before running the app.
    # The quotes are crucial for paths with spaces and for the whole command string.
    # We explicitly expand %PATH% and %GDAL_DRIVER_PATH% so they are evaluated *within* cmd.exe
    add_custom_target(run_app
        COMMAND cmd /c "set \"PATH=${RANDOM_RASTER_DLL_DIR};%PATH%\" && set \"GDAL_DRIVER_PATH=${RANDOM_RASTER_DLL_DIR};%GDAL_DRIVER_PATH%\" && \"$<TARGET_FILE:random_raster_app>\""
        DEPENDS gdal_RANDOM_RASTER random_raster_app
        USES_TERMINAL # Important for seeing stdout/stderr and interactive prompts
        COMMENT "Running test app with GDAL_DRIVER_PATH extended for Windows..."
    )
else()
    # On Unix-like systems (Linux, macOS), use sh -c to set the environment variable and run
    # Use && for chaining. The syntax for appending to PATH/GDAL_DRIVER_PATH is different.
    add_custom_target(run_app
        COMMAND sh -c "export PATH=${RANDOM_RASTER_DLL_DIR}:${PATH} && export GDAL_DRIVER_PATH=${RANDOM_RASTER_DLL_DIR}:${GDAL_DRIVER_PATH} && \"$<TARGET_FILE:random_raster_app>\""
        DEPENDS gdal_RANDOM_RASTER random_raster_app
        USES_TERMINAL
        COMMENT "Running test app with GDAL_DRIVER_PATH extended for Unix..."
    )
endif()