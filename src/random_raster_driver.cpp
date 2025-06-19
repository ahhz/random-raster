//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================
//

#include <gdal_priv.h>
#include <pronto/raster/random_raster_dataset.h>
#include <pronto/raster/random_raster_driver.h>

namespace pronto {
  namespace raster {
	

    void register_random_raster_driver()
    {
      if (GDALGetDriverByName("RANDOM_RASTER") != nullptr)
        return;

      GDALDriver* driver = new GDALDriver();

      driver->SetDescription("RANDOM_RASTER");
      driver->SetMetadataItem(GDAL_DMD_LONGNAME, "Random Distribution Raster");
      driver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "https://github.com/ahhz/random-raster/blob/main/docs/random_raster_driver.md");
      driver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
      driver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
      driver->SetMetadataItem(GDAL_DMD_EXTENSION, "json"); 

      driver->pfnOpen = random_raster_dataset::Open;
      driver->pfnIdentifyEx = random_raster_dataset::IdentifyEx;

      GetGDALDriverManager()->RegisterDriver(driver);
    }

  } // namespace raster 
} // namespace pronto

// Register the driver with GDAL when the library is loaded.
extern "C" {
  void GDALRegister_RANDOM_RASTER()
  {
    pronto::raster::register_random_raster_driver();
  }
}