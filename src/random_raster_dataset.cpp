//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================
//

#include <iostream>
#include <memory.h>

#include <gdal_priv.h>
#include <ogr_spatialref.h> // For OGRSpatialReference

#include <pronto/raster/block_generator_interface.h> 
#include <pronto/raster/random_raster_dataset.h>
#include <pronto/raster/random_raster_band.h>


#define PRONTO_RASTER_MAX_JSON_FILE_SIZE (10 * 1024 * 1024) // 10 MB limit

namespace pronto {
  namespace raster {

    // --- random_raster_band Implementation ---

    // Constructor.
    // Private constructor.
    random_raster_dataset::random_raster_dataset(int rows, int cols, GDALDataType data_type,
        int block_rows, int block_cols, std::unique_ptr<block_generator_interface>&& block_generator)
        : m_block_generator(std::move(block_generator))
    {
      nRasterXSize = cols;
      nRasterYSize = rows;
      SetBand(1, new random_raster_band(this, 1, m_block_generator.get(),
        data_type, block_rows, block_cols));
    }

    // Creates a random_raster_dataset instance.
    random_raster_dataset* random_raster_dataset::create(const random_raster_parameters& params)
    {
      // Assumes 'params' is already validated.
      std::unique_ptr<block_generator_interface> generator = params.create_block_generator();

      if (generator == nullptr) {
        return nullptr;
      }

      return new random_raster_dataset(params.m_rows, params.m_cols, params.m_data_type,
        params.m_block_rows, params.m_block_cols, std::move(generator));
    }

    // GDAL driver entry point for opening datasets.
    GDALDataset* random_raster_dataset::Open(GDALOpenInfo* openInfo)
    {
	    std::cout << "Opening file: " << openInfo->pszFilename << std::endl;
      std::string content_to_parse;

      // 1. Check if the input is a file path and if it matches our criteria
      VSIStatBufL sStatBuf;
      if (VSIStatL(openInfo->pszFilename, &sStatBuf) == 0 && VSI_ISREG(sStatBuf.st_mode)) {
        // It's a regular file. Check extension and size.
        std::string filename_str = openInfo->pszFilename;
        if (filename_str.length() < 5 || !EQUAL(filename_str.c_str() + filename_str.length() - 5, ".json"))
        {
          std::cout << "Not a.json file" << std::endl;

          return nullptr; // Not a .json file.
        }

        if (sStatBuf.st_size > PRONTO_RASTER_MAX_JSON_FILE_SIZE) {
          CPLError(CE_Debug, CPLE_AppDefined, "JSON parameter file too large (%lld bytes): %s",
            static_cast<long long>(sStatBuf.st_size), openInfo->pszFilename);
          return nullptr; // File too large.
        }

        // Read file content.
        VSILFILE* fp = VSIFOpenL(openInfo->pszFilename, "rb");
        if (fp == nullptr) {
          std::cout << "File can't be opened" << std::endl;
          return nullptr; // File can't be opened.
        }

        content_to_parse.resize(sStatBuf.st_size);
        size_t bytes_read = VSIFReadL(&content_to_parse[0], 1, sStatBuf.st_size, fp);
        VSIFCloseL(fp);

        if (bytes_read != (size_t)sStatBuf.st_size) {
          std::cout << "Incomplete Read" << std::endl;
          CPLError(CE_Debug, CPLE_AppDefined, "Incomplete read of JSON parameter file: %s", openInfo->pszFilename);
          return nullptr; // Incomplete read.
        }
      }
      else {
        std::cout << "Not a regular files, assume it is a JSON string" <<std::endl;

        // Not a regular file, assume the filename string itself is the JSON content.
        content_to_parse = openInfo->pszFilename;
      }

      // 2. Attempt to parse the content as JSON.
      nlohmann::json j;
      try {
        j = nlohmann::json::parse(content_to_parse);
      }
      catch (const nlohmann::json::exception&) {
		  std::cout << "Failed to parse JSON content: " << content_to_parse << std::endl;
        return nullptr; // Malformed JSON.
      }

      // 3. Check if the JSON is specifically for RANDOM_RASTER.
      if (!is_random_raster_json(j)) {
        return nullptr; // Valid JSON, but not our 'type'.
      }

      // 4. Populate parameters and perform full validation.
      random_raster_parameters params;
      if (!params.from_json(j)) {
        CPLError(CE_Debug, CPLE_AppDefined, "JSON for RANDOM_RASTER but content validation failed. Message: %s", CPLGetLastErrorMsg());
        CPLErrorReset();
        return nullptr;
      }

      return random_raster_dataset::create(params);
    }

    CPLErr random_raster_dataset::GetGeoTransform(double* padfTransform)
    {
      // A default GeoTransform: 1x1 pixel size, no rotation, origin at (0,0)
      padfTransform[0] = 0.0;  // Top-left X
      padfTransform[1] = 1.0;  // W-E pixel resolution
      padfTransform[2] = 0.0;  // Rotation, 0 if image is "north up"
      padfTransform[3] = 0.0;  // Top-left Y
      padfTransform[4] = 0.0;  // Rotation, 0 if image is "north up"
      padfTransform[5] = -1.0; // N-S pixel resolution (negative value)
      return CE_None;
    }

    const OGRSpatialReference* random_raster_dataset::GetSpatialRef() const
    {
      // Return nullptr for an unknown spatial reference system
      return nullptr;
    }

  } // namespace raster 
} // namespace pronto

