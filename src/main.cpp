#include <iostream>
#include <string>

#include <gdal.h>
#include <gdal_priv.h>

int main() {
  GDALAllRegister();

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

  GDALDataset* dataset = (GDALDataset * )GDALOpen(json_params, GA_ReadOnly);

  if (dataset == nullptr) {
    std::cerr << "Error: Could not open dataset.  " << CPLGetLastErrorMsg() << std::endl;
    return 1;
  }

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
  int size_of_element = 1; // GDT_Byte is 1 byte
  GByte* block_data = new GByte[block_x_size * block_y_size * size_of_element];
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