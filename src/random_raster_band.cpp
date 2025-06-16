#include <pronto/raster/random_dataset.h>
#include <pronto/raster/random_raster_band.h>
#include <pronto/raster/block_generator_interface.h> 

namespace pronto {
namespace raster {

// Constructor.
random_raster_band::random_raster_band(random_dataset* ds, int n_band, block_generator_interface* block_gen,
                                   GDALDataType data_type, int block_x_size, int block_y_size)
    : m_block_generator(block_gen)
{
  poDS = ds;
  nBand = n_band;
  eDataType = data_type;
  this->nBlockXSize = block_x_size;
  this->nBlockYSize = block_y_size;
}

// Reads a block of data.
CPLErr random_raster_band::IReadBlock(int nBlockXOff, int nBlockYOff, void* p_data)
{
  if (m_block_generator == nullptr) {
    CPLError(CE_Failure, CPLE_AppDefined, "Block generator is null for random_raster_band.");
    return CE_Failure;
  }

  const int pixels_in_block = nBlockXSize * nBlockYSize;
  m_block_generator->fill_block_void(nBlockYOff, nBlockXOff, p_data, pixels_in_block);

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