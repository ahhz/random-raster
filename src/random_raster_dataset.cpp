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
#include <pronto/raster/random_raster_parameters.h>
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
    nlohmann::json read_json_from_GDALOpenInfo(GDALOpenInfo* openInfo)
    {
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
          std::string msg = std::string( "File/resource can't be opened: ") + openInfo->pszFilename;
          throw std::runtime_error(msg);
        }
        VSIFSeekL(fp, 0, SEEK_END);
        vsi_l_offset file_size = VSIFTellL(fp);
        VSIFSeekL(fp, 0, SEEK_SET);
        if (file_size > PRONTO_RASTER_MAX_JSON_FILE_SIZE) {
          CPLError(CE_Failure, CPLE_AppDefined, "JSON file too large (%lld bytes) for in-memory parsing: %s",
            static_cast<long long>(file_size), openInfo->pszFilename);
          VSIFCloseL(fp);
          std::string msg = std::string("JSON file too large for parsing as a RANDOM_RASTER");
          throw std::runtime_error(msg);
        }
        content_to_parse.resize(static_cast<size_t>(file_size));
        size_t bytes_read = VSIFReadL(&content_to_parse[0], 1, static_cast<size_t>(file_size), fp);
        VSIFCloseL(fp);

        if (bytes_read != static_cast<size_t>(file_size)) {
          std::string msg = std::string("JSON file not completely read when parsing as a RANDOM_RASTER");
          throw std::runtime_error(msg);
        }
      }
      else {
        std::string msg = std::string("No data source (filename or buffer) provided.");
        throw std::runtime_error(msg);
      }

      // --- JSON Parsing ---
      return nlohmann::json::parse(content_to_parse);
    }

    
    int random_raster_dataset::Identify(GDALOpenInfo* openInfo)
    {
      try {
        auto j = read_json_from_GDALOpenInfo(openInfo);
        if (j.contains("type") && j["type"] == "RANDOM_RASTER") {
          return TRUE; // Identified as a RANDOM_RASTER type
        }
      } catch(std::exception& e){
        // clear exceptions, should not throw, but quitely return FALSE
      }
      return FALSE;

    }
    GDALDataset* random_raster_dataset::create_from_generator(
      int rows, int cols, GDALDataType data_type,
      int block_rows, int block_cols, std::unique_ptr<block_generator_interface>&& block_generator)
    {
      return new random_raster_dataset(rows, cols, data_type, 
        block_rows, block_cols, std::move(block_generator));
    };

    // GDAL driver entry point for opening datasets.
    GDALDataset* random_raster_dataset::Open(GDALOpenInfo* openInfo)
    {
      if (!random_raster_dataset::Identify(openInfo)) {
        return nullptr; // Not a random raster dataset, quietly return nullptr.
      }
      try {
        nlohmann::json j = read_json_from_GDALOpenInfo(openInfo);
        random_raster_dataset* poDS = static_cast<random_raster_dataset*>(create_from_json(j));;

        // Set the virtual flag and PAM description based on source type

        const bool is_purely_in_memory_buffer = openInfo->pabyHeader != nullptr && openInfo->nHeaderBytes > 0;
        std::string dataset_id = ""; // Name for GDAL's description/PAM
        if (is_purely_in_memory_buffer)
        {
         dataset_id = is_purely_in_memory_buffer  
           ? "random_raster_in_memory_data" 
           : openInfo->pszFilename; 
         }
        poDS->m_bIsVirtual = is_purely_in_memory_buffer; // Simpler assignment
        poDS->SetDescription(dataset_id.c_str());
        if (!is_purely_in_memory_buffer) {
          poDS->TryLoadXML(openInfo->GetSiblingFiles());
        }
        return poDS;
      }
      catch (const std::exception& e) {

        CPLError(CE_Failure, CPLE_AppDefined, "Failed to open random raster dataset: %s", e.what());
        return nullptr; // Return nullptr on failure
      }
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

