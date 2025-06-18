//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//===
#include <pronto/raster/block_generator_interface.h> 
#include <pronto/raster/random_raster_dataset.h>
#include <pronto/raster/random_raster_band.h>

namespace pronto {
  namespace raster {

    random_raster_band::random_raster_band(random_raster_dataset* ds, int n_band, block_generator_interface* block_gen,
                                       GDALDataType data_type, int block_rows, int block_cols)
        : m_block_generator(block_gen)
    {
      poDS = ds;
      nBand = n_band;
      eDataType = data_type;
      this->nBlockXSize = block_cols;
      this->nBlockYSize = block_rows;
    }

    CPLErr random_raster_band::IReadBlock(int nBlockXOff, int nBlockYOff, void* p_data)
    {
      if (m_block_generator == nullptr) {
        CPLError(CE_Failure, CPLE_AppDefined, "Block generator is null for random_raster_band.");
        return CE_Failure;
      }
      int major_row = nBlockYOff; 
      int major_col = nBlockYOff;
      const int pixels_in_block = nBlockXSize * nBlockYSize;
      m_block_generator->fill_block(major_row, major_col, p_data, pixels_in_block);

      return CE_None;
    }

    // Returns the minimum possible value.
    double random_raster_band::GetMinimum(int* pbSuccess)
    {
      if (pbSuccess) *pbSuccess = TRUE;
      if (m_block_generator == nullptr) {
        CPLError(CE_Failure, CPLE_AppDefined, "Block generator is null when getting minimum.");
        if (pbSuccess) *pbSuccess = FALSE;
        return 0.0;
      }
      return m_block_generator->get_min();
    }

    // Returns the maximum possible value.
    double random_raster_band::GetMaximum(int* pbSuccess)
    {
      if (pbSuccess) *pbSuccess = TRUE;
      if (m_block_generator == nullptr) {
        CPLError(CE_Failure, CPLE_AppDefined, "Block generator is null when getting maximum.");
        if (pbSuccess) *pbSuccess = FALSE;
        return 1.0;
      }
      return m_block_generator->get_max();
    }

    // Provides basic statistics.
    CPLErr random_raster_band::GetStatistics(int bApproxOK, int bForce, double* pdfMin, double* pdfMax,
                                           double* pdfMean, double* pdfStdDev)
    {
      if (m_block_generator == nullptr) {
        CPLError(CE_Failure, CPLE_AppDefined, "Block generator is null when getting statistics.");
        return CE_Failure;
      }

      if (pdfMin) *pdfMin = m_block_generator->get_min();
      if (pdfMax) *pdfMax = m_block_generator->get_max();
      if (pdfMean) *pdfMean = m_block_generator->get_mean();
      if (pdfStdDev) *pdfStdDev = m_block_generator->get_std_dev();

      return CE_None;
    }
  } // namespace raster
} // namespace pronto