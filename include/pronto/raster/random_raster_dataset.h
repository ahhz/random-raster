//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================
//
#pragma once

#include <iostream>
#include <memory>

#include <gdal_pam.h>
#include <gdal_priv.h>

#include <pronto/raster/block_generator_interface.h> 
#include <pronto/raster/random_raster_parameters.h> 

namespace pronto {
  namespace raster {
    class random_raster_parameters; //forward declaration
    class random_raster_dataset : public GDALPamDataset
    {
    private:
      // The owned block generator.
      std::unique_ptr<block_generator_interface> m_block_generator;
  
      // Private constructor for internal use by factory methods.
      random_raster_dataset(int rows, int cols, GDALDataType data_type,
                    int block_rows, int block_cols, std::unique_ptr<block_generator_interface>&& block_generator);

    public:
      random_raster_dataset() = delete; // Disable default constructor.
      ~random_raster_dataset() override = default;
      static int IdentifyEx(GDALDriver* poDriver, GDALOpenInfo* openInfo)
      {
	      std::cout << "IdentifyEx called for file: " << openInfo->pszFilename << std::endl;
        std::string content_to_parse;

        // First, try to handle it as a regular file if openInfo->pszFilename exists
        VSIStatBufL sStatBuf;
        if (openInfo->pszFilename != nullptr && VSIStatL(openInfo->pszFilename, &sStatBuf) == 0 && VSI_ISREG(sStatBuf.st_mode)) {
          // It's a regular file. Check extension.
          std::string filename_str = openInfo->pszFilename;
          if (filename_str.length() >= 5 && EQUAL(filename_str.c_str() + filename_str.length() - 5, ".json")) {
            // It's a .json file, attempt to read a small header for identification.
            // Avoid reading the entire file here, just enough to check 'is_random_raster_json'
            VSILFILE* fp = VSIFOpenL(openInfo->pszFilename, "rb");
            if (fp == nullptr) {
              return FALSE;
            }
            char header_buf[2048]; // Read a sensible amount for identification
            size_t bytes_read = VSIFReadL(header_buf, 1, std::min((size_t)2047, (size_t)sStatBuf.st_size), fp);
            VSIFCloseL(fp);

            if (bytes_read > 0) {
              header_buf[bytes_read] = '\0'; // Null-terminate for string operations
              try {
                nlohmann::json j = nlohmann::json::parse(header_buf);
                if (is_random_raster_json(j)) {
                  return TRUE; // Identified!
                }
              }
              catch (const nlohmann::json::exception&) {
                // Not valid JSON, or not enough to parse.
              }
            }
          }
          return FALSE; // Not a .json file or not identifiable as our JSON
        }

        // If it's not a regular file, assume the filename string itself is the JSON content.
        if (openInfo->pszFilename != nullptr && openInfo->pszFilename[0] == '{') {
          try {
            nlohmann::json j = nlohmann::json::parse(openInfo->pszFilename);
            if (is_random_raster_json(j)) {
              return TRUE; // Identified!
            }
          }
          catch (const nlohmann::json::exception&) {
            // Not valid JSON, or not our type.
          }
        }

        return FALSE; // Not recognized
      }

      // --- Public Static Factory Method for Direct C++ Instantiation ---
      // Creates a random_dataset instance using parameters.
      static random_raster_dataset* create(const random_raster_parameters& params);

        // Static factory method for opening datasets via GDAL's driver system.
      // Parses a virtual filename string and delegates to the create factory.
      static GDALDataset* Open(GDALOpenInfo* openInfo);

      // Required overrides for GDALDataset to provide basic spatial information.
      CPLErr GetGeoTransform(double* padfTransform) override;
      const OGRSpatialReference* GetSpatialRef() const override;
    };

  } // namespace raster
} // namespace pronto
