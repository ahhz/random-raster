cmake_minimum_required(VERSION 3.20)

# This CMake file now works "out of the box" in conjunction with vcpkg in manifest mode, along with the vcpkg.json file

project(pronto_random_raster)

################################################################
# Creating the main library: Pronto Raster
#

# Require C++20, std:c++latest for msvc and -std=c++20 for everyone else
if (MSVC)
    # this change happened in CMake 3.20.4
    set(PRONTO_RANDOM_RASTER_CXX_STANDARD 23) # /std:c++latest - unlocks the non stable cpp20 features. For new 16.11 versions
else ()
    set(PRONTO_RANDOM_RASTER_STANDARD 20) # /std:c++latest for msvc and -std=c++20 for everyone else.
endif ()

set(pronto_random_raster_files
	${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/block_generator_interface.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_block_generator.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_dataset.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_driver.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_raster_band.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/pronto/raster/random_raster_parameters.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/random_dataset.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/random_driver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_band.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/random_raster_parameters.cpp
)

add_library(random_raster ${pronto_random_raster_files})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/ )
set_target_properties(random_raster PROPERTIES CXX_STANDARD ${PRONTO_RANDOM_RASTER_CXX_STANDARD})
set_target_properties(random_raster PROPERTIES LINKER_LANGUAGE CXX)
# Set include directory, 
target_include_directories(random_raster INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/ )

# take care of GDAL dependency
find_package(GDAL CONFIG REQUIRED)
target_link_libraries(random_raster PUBLIC GDAL::GDAL)

find_package(nlohmann_json REQUIRED)
target_link_libraries(random_raster PRIVATE nlohmann_json::nlohmann_json)

