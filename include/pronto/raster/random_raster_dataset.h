//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================
//
#pragma once

#include <memory>

#include <gdal_pam.h>
#include <gdal_priv.h>

#include <nlohmann/json.hpp>

#include <pronto/raster/block_generator_interface.h> 

namespace pronto {
  namespace raster {
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

      static GDALDataset* create_from_generator(
        int rows, int cols, GDALDataType data_type,
        int block_rows, int block_cols, std::unique_ptr<block_generator_interface>&& block_generator);

      static GDALDataset* create_from_json(const nlohmann::json& json_params);
      static int Identify(GDALOpenInfo* openInfo);
      static GDALDataset* Open(GDALOpenInfo* openInfo);
      CPLErr GetGeoTransform(double* padfTransform) override;
      const OGRSpatialReference* GetSpatialRef() const override;
      bool m_bIsVirtual;
    };

  } // namespace raster
} // namespace pronto
