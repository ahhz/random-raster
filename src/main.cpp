#include <iostream>
#include <string>

#include <gdal.h>
#include <gdal_priv.h>

int main() {
  // 1. Register GDAL drivers (including our custom driver).
    const char* gdalDriverPath = getenv("GDAL_DRIVER_PATH");

    if (gdalDriverPath != nullptr) {
        std::cout << "GDAL_DRIVER_PATH is: " << gdalDriverPath << std::endl;
    }
    else {
        std::cout << "GDAL_DRIVER_PATH is NOT set." << std::endl;
    }
  GDALAllRegister();
  std::cout << "GDAL Registered Drivers:" << std::endl;

  // Get the total number of registered drivers
   int driverCount = GDALGetDriverCount();

  // Iterate through each driver
  for (int i = 0; i < driverCount; ++i) {
      GDALDriver* poDriver = (GDALDriver*)GDALGetDriver(i);
      if (poDriver) {
          std::cout << "  - " << poDriver->GetDescription() << std::endl;
      }
  }
  // Optional: Clean up GDAL resources (though often not strictly necessary at program exit)
  //GDALDestroyDriverManager();
  //pronto::raster::register_random_driver();

  // 2. Define the parameters for the random raster.  This could be read from a file,
  //    constructed programmatically, etc.  For this example, we'll use a simple JSON string.
  const char* json_params =
    "{"
    "  \"type\": \"RANDOM_RASTER\","
    "  \"rows\": 256,"
    "  \"cols\": 512,"
    "  \"data_type\": \"GDT_Byte\","
    "  \"seed\": 1234,"
    "  \"block_rows\": 64,"
    "  \"block_cols\": 64,"
    "  \"distribution\": \"uniform_integer\","
    "  \"distribution_parameters\": {"
    "    \"a\": 1,"
    "    \"b\": 6"
    "  }"
    "}";

  const char* json_file = "demo.json";
  // 3.  Parse the JSON parameters into our custom parameter struct.
  //pronto::raster::random_raster_parameters params;
  //if (!params.from_string_json(json_params)) {
  //  std::cerr << "Error: Failed to parse JSON parameters." << std::endl;
  // return 1;
  //}

  // 4. Create the virtual filename.  In this case, we're just passing the JSON
  //    string directly as the filename.  GDAL will treat this as a virtual file.
  const char* virtual_filename = json_params;
  GDALDataset* dataset = (GDALDataset * )GDALOpen(virtual_filename, GA_ReadOnly);

  if (dataset == nullptr) {
    std::cerr << "Error: Could not open dataset.  " << CPLGetLastErrorMsg() << std::endl;
    return 1;
  }

  // 6.  Access the raster band and read some data.
  GDALRasterBand* band = dataset->GetRasterBand(1);
  if (band == nullptr) {
    std::cerr << "Error: Could not get raster band." << std::endl;
    GDALClose(dataset);
    return 1;
  }

  int width = band->GetXSize();
  int height = band->GetYSize();
  std::cout << "Raster width: " << width << ", height: " << height << std::endl;

  // Read a block of data (e.g., the first block).
  int block_x_size, block_y_size;
  band->GetBlockSize(&block_x_size, &block_y_size);
  std::cout << "Block width: " << block_x_size << ", height: " << block_y_size << std::endl;

  // Allocate a buffer to hold the block data.
  GByte* block_data = new GByte[block_x_size * block_y_size];
  CPLErr err = band->ReadBlock(0, 0, block_data); // Block offset (0,0)
  if (err != CE_None) {
    std::cerr << "Error: Could not read block." << std::endl;
    delete[] block_data;
    GDALClose(dataset);
    return 1;
  }

  // Print a few values from the block.
  std::cout << "First few values from the block:" << std::endl;
  for (int i = 0; i < std::min(10, block_x_size * block_y_size); ++i) {
    std::cout << static_cast<int>(block_data[i]) << " ";
  }
  std::cout << std::endl;

  // Clean up.
  delete[] block_data;
  GDALClose(dataset);

  return 0;
}