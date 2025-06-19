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
   
      // --- Public Static Factory Method for Direct C++ Instantiation ---
      // Creates a random_dataset instance using parameters.
      static random_raster_dataset* create(const random_raster_parameters& params);

      // GDAL Required functions:
      static int IdentifyEx(GDALDriver* poDriver, GDALOpenInfo* openInfo);
      static GDALDataset* Open(GDALOpenInfo* openInfo);
      CPLErr GetGeoTransform(double* padfTransform) override;
      const OGRSpatialReference* GetSpatialRef() const override;
    };

  } // namespace raster
} // namespace pronto
