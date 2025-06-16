#include <pronto/raster/random_driver.h>
#include <gdal_priv.h>

namespace pronto {
namespace raster {
	
void register_random_driver()
{
  if (GDALGetDriverByName("RANDOM_RASTER") != nullptr)
    return;

  GDALDriver* driver = new GDALDriver();

  driver->SetDescription("RANDOM_RASTER");
  driver->SetMetadataItem(GDAL_DMD_LONGNAME, "Randomly Generated Raster");
  driver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "https://gdal.org");
  //driver->SetMetadataItem(GDAL_DMD_SYNCMETADATA, "NO");
  driver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

  driver->pfnOpen = random_dataset::Open;

  GDALDriverManager().RegisterDriver(driver);
}

} // namespace raster
} // namespace pronto