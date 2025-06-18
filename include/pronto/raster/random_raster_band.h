//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================
//
#pragma once

#include <gdal_priv.h>
#include <gdal_pam.h>

#include <pronto/raster/block_generator_interface.h>
#include <pronto/raster/random_raster_dataset.h>
#include <pronto/raster/random_raster_band.h>

namespace pronto {
  namespace raster {

    // --- random_raster_band Class Definition ---
    // Represents a single band of a random_dataset, delegates data generation and statistics.
    class random_raster_band : public GDALPamRasterBand
    {
    private:
      // Non-owning pointer to the block generator owned by random_dataset.
      block_generator_interface* m_block_generator;

    protected:
      CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* p_data) override;

    public:
      random_raster_band(
        random_raster_dataset* ds, 
        int n_band, 
        block_generator_interface* block_gen,
        GDALDataType data_type, 
        int block_rows, int block_cols);
      
      ~random_raster_band() override = default;

      // Overrides for GDALRasterBand properties, delegated to m_block_generator.
      double GetMinimum(int* pbSuccess = nullptr) override;
      double GetMaximum(int* pbSuccess = nullptr) override;
      CPLErr GetStatistics(int bApproxOK, int bForce, double* pdfMin, double* pdfMax,
          double* pdfMean, double* pdfStdDev) override;
     };

  } // namespace raster
} // namespace pronto
