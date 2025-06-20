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
#include <pronto/raster/random_raster_band.h>
#include <pronto/raster/random_raster_dataset.h>

#define PRONTO_RASTER_MAX_JSON_FILE_SIZE (10 * 1024 * 1024) // 10 MB limit

namespace pronto {
  namespace raster {

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
      std::unique_ptr<block_generator_interface> generator;
      try {
        generator = params.create_block_generator();
      }
      catch (const std::exception& e){
        CPLError(CE_Failure, CPLE_AppDefined, "JSON parsing error: %s", e.what());
        generator = nullptr;
      }

      if (generator == nullptr) {
        return nullptr;
      }

      return new random_raster_dataset(params.m_rows, params.m_cols, params.m_data_type,
        params.m_block_rows, params.m_block_cols, std::move(generator));
    }
    int random_raster_dataset::IdentifyEx(GDALDriver* poDriver, GDALOpenInfo* openInfo)
    {
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
    // GDAL driver entry point for opening datasets.
    GDALDataset* random_raster_dataset::Open(GDALOpenInfo* openInfo)
    {
      if (!random_raster_dataset::IdentifyEx(nullptr, openInfo)) {
        return nullptr; // Not a random raster dataset.
      }
      // Simplify boolean declaration for clarity
      const bool is_purely_in_memory_buffer = openInfo->pabyHeader != nullptr && openInfo->nHeaderBytes > 0;
      std::string content_to_parse;
      std::string dataset_id = ""; // Name for GDAL's description/PAM
      if (is_purely_in_memory_buffer)
      {
        content_to_parse.assign(reinterpret_cast<const char*>(openInfo->pabyHeader), openInfo->nHeaderBytes);
        dataset_id = "random_raster_in_memory_data"; // Generic ID for in-memory
      }
      else if (openInfo->pszFilename != nullptr && strlen(openInfo->pszFilename) > 0)
      {
        dataset_id = openInfo->pszFilename; // Filename is the ID for PAM and dataset description
        VSILFILE* fp = VSIFOpenL(openInfo->pszFilename, "rb");

        if (fp == nullptr) {
          CPLError(CE_Failure, CPLE_AppDefined, "File/resource can't be opened: %s", openInfo->pszFilename);
          return nullptr;
        }
        VSIFSeekL(fp, 0, SEEK_END);
        vsi_l_offset file_size = VSIFTellL(fp);
        VSIFSeekL(fp, 0, SEEK_SET);
        if (file_size > PRONTO_RASTER_MAX_JSON_FILE_SIZE) {
          CPLError(CE_Failure, CPLE_AppDefined, "JSON file too large (%lld bytes) for in-memory parsing: %s",
            static_cast<long long>(file_size), openInfo->pszFilename);
          VSIFCloseL(fp);
          return nullptr;
        }
        content_to_parse.resize(static_cast<size_t>(file_size));
        size_t bytes_read = VSIFReadL(&content_to_parse[0], 1, static_cast<size_t>(file_size), fp);
        VSIFCloseL(fp);

        if (bytes_read != static_cast<size_t>(file_size)) {
          CPLError(CE_Failure, CPLE_AppDefined, "Incomplete read of JSON parameter file: %s", openInfo->pszFilename);
          return nullptr;
        }
      }

      else {
        CPLError(CE_Failure, CPLE_AppDefined, "No data source (filename or buffer) provided.");
        return nullptr;
      }

      // --- JSON Parsing ---
      nlohmann::json j;

      try {
        j = nlohmann::json::parse(content_to_parse);
      }
      catch (const nlohmann::json::exception& e) {
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to parse JSON content from '%s', malformed JSON: %s. Error: %s",

          dataset_id.empty() ? "unknown_source" : dataset_id.c_str(), content_to_parse.c_str(), e.what());
        return nullptr;
      }
      // --- Parameters Validation ---

      random_raster_parameters params;

      if (!params.from_json(j)) {
        CPLError(CE_Failure, CPLE_AppDefined, "JSON for RANDOM_RASTER but content parsing failed. Message: %s", CPLGetLastErrorMsg());
        return nullptr;

      }
      if (!params.validate()) {
        CPLError(CE_Failure, CPLE_AppDefined, "JSON for RANDOM_RASTER but parameters validation failed. Message: %s", CPLGetLastErrorMsg());
        return nullptr;
      }

      random_raster_dataset* poDS = random_raster_dataset::create(params);

      if (poDS == nullptr) {
        return nullptr;
      }
      // Set the virtual flag and PAM description based on source type
      poDS->m_bIsVirtual = is_purely_in_memory_buffer; // Simpler assignment
      poDS->SetDescription(dataset_id.c_str());
      if (!is_purely_in_memory_buffer) {
        poDS->TryLoadXML(openInfo->GetSiblingFiles());
      }
      return poDS;

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

