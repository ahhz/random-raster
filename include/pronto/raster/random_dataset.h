#pragma once

#include <gdal_priv.h>
#include <gdal_pam.h>
#include <pronto/raster/block_generator_interface.h> 
#include <pronto/raster/random_raster_parameters.h> 

namespace pronto {
namespace raster {
class random_raster_parameters; //forward declaration
class random_dataset : public GDALPamDataset
{
private:
  // The owned block generator.
  std::unique_ptr<block_generator_interface> m_block_generator;
  
  // Private constructor for internal use by factory methods.
  random_dataset(int raster_x_size, int raster_y_size, GDALDataType data_type,
                int block_x_size, int block_y_size, std::unique_ptr<block_generator_interface>&& block_generator);

public:
  random_dataset() = delete; // Disable default constructor.
  ~random_dataset() override = default;

  // --- Public Static Factory Method for Direct C++ Instantiation ---
  // Creates a random_dataset instance using parameters.
  static random_dataset* create(const random_raster_parameters& params);

    // Static factory method for opening datasets via GDAL's driver system.
  // Parses a virtual filename string and delegates to the create factory.
  static GDALDataset* Open(GDALOpenInfo* openInfo);

  // Required overrides for GDALDataset to provide basic spatial information.
  //CPLErr GetGeoTransform(double* padfTransform) override;
  //const char* GetProjectionRef(void) const;// override;
};

} // namespace raster
} // namespace pronto