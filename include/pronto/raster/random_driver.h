#pragma once

#include "gdal_priv.h"
#include "random_dataset.h" // For random_dataset::Open

namespace pronto {
namespace raster {

// --- GDALDriver Registration Function ---
// Registers the "RANDOM_RASTER" driver with GDAL.
void register_random_driver();

} // namespace raster
} // namespace pronto
