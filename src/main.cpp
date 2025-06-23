#include <iostream> 
#include <string> 
#include <algorithm> 
#include <vector>    

#include <gdal.h>      
#include <gdal_priv.h> 
#include <cpl_vsi.h>   
#include <cpl_error.h> 
#include <cpl_conv.h>  

#ifndef RANDOM_RASTER_DRIVER_PATH
#define RANDOM_RASTER_DRIVER_PATH "path/to/the/gdal_RANDOM_RASTER_shared_library/directory" // Consider making this an env var or cmd line arg
#endif

// Function to check if the RANDOM_RASTER driver is installed
bool check_driver_installed() {
  GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("RANDOM_RASTER");
  return (driver != nullptr);
}

// Function to manually install the RANDOM_RASTER driver
bool install_driver_manually() {
  const char* restore_path = CPLGetConfigOption("GDAL_DRIVER_PATH", nullptr);
  CPLSetConfigOption("GDAL_DRIVER_PATH", RANDOM_RASTER_DRIVER_PATH);
  GetGDALDriverManager()->AutoLoadDrivers(); // This will load drivers from RANDOM_RASTER_DRIVER_PATH
  CPLSetConfigOption("GDAL_DRIVER_PATH", restore_path); // Restore original path
  return check_driver_installed();
}

int main() {
  GDALAllRegister(); // Register all GDAL drivers

  if (check_driver_installed()) {
    std::cout << "RANDOM_RASTER driver automatically registered successfully!" << std::endl;
  }
  else if (install_driver_manually()) {
    std::cout << "RANDOM_RASTER driver manually registered successfully!" << std::endl;
  }
  else {
    std::cerr << "RANDOM_RASTER driver not found. Please ensure RANDOM_RASTER_DRIVER_PATH is correct and the driver is built." << std::endl;
    std::cerr << "Current RANDOM_RASTER_DRIVER_PATH set to: " << RANDOM_RASTER_DRIVER_PATH << std::endl;
    GDALDestroyDriverManager(); 
    return 1; 
  }

  const std::string json_params =
    "{\n"
    "  \"type\": \"RANDOM_RASTER\",\n"
    "  \"rows\": 256,\n"
    "  \"cols\": 512,\n"
    "  \"data_type\": \"Byte\",\n"
    "  \"seed\": 1234,\n"
    "  \"block_rows\": 64,\n"
    "  \"block_cols\": 64,\n"
    "  \"distribution\": \"uniform_integer\",\n"
    "  \"distribution_parameters\": {\n"
    "    \"a\": 1,\n"
    "    \"b\": 6\n"
    "  }\n"
    "}";

  // --- Start of VSI Memory File Integration ---
  const char* vsi_mem_filename = "/vsimem/random_raster_params.json";

  VSILFILE* fp_vsimem = VSIFileFromMemBuffer(
    vsi_mem_filename,
    reinterpret_cast<GByte*>(const_cast<char*>(json_params.c_str())),
    json_params.length(),
    FALSE
  );

  if (fp_vsimem == nullptr) {
    std::cerr << "Error: Could not create VSI memory file for JSON parameters." << std::endl;
    std::cerr << "GDAL Last Error: " << CPLGetLastErrorMsg() << std::endl;
    GDALDestroyDriverManager();
    return 1;
  }
  VSIFCloseL(fp_vsimem);

  GDALDataset* dataset = static_cast<GDALDataset*>(GDALOpen(vsi_mem_filename, GA_ReadOnly));

  if (dataset == nullptr) {
    std::cerr << "Error: Could not open dataset from VSI memory file. " << CPLGetLastErrorMsg() << std::endl;
    VSIUnlink(vsi_mem_filename); 
    GDALDestroyDriverManager();
    return 1;
  }
  // --- End of VSI Memory File Integration ---

  GDALRasterBand* band = dataset->GetRasterBand(1);
  if (band == nullptr) {
    std::cerr << "Error: Could not get raster band." << std::endl;
    GDALClose(dataset);
    VSIUnlink(vsi_mem_filename); 
    GDALDestroyDriverManager();
    return 1;
  }

  int width = band->GetXSize();
  int height = band->GetYSize();
  std::cout << "Raster width: " << width << ", height: " << height << std::endl;

  int block_x_size, block_y_size;
  band->GetBlockSize(&block_x_size, &block_y_size);
  std::cout << "Block width: " << block_x_size << ", height: " << block_y_size << std::endl;

  const int size_of_element = 1;
  std::vector<GByte> block_data(static_cast<size_t>(block_x_size) * block_y_size * size_of_element);

  CPLErr err = band->ReadBlock(0, 0, block_data.data());
  if (err != CE_None) {
    std::cerr << "Error: Could not read block: " << CPLGetLastErrorMsg() << std::endl;
    GDALClose(dataset);
    VSIUnlink(vsi_mem_filename);
    GDALDestroyDriverManager();
    return 1;
  }

  std::cout << "First few values from the block (should be between 1 and 6):" << std::endl;
  for (int i = 0; i < std::min(10, block_x_size * block_y_size); ++i) {
    std::cout << static_cast<int>(block_data[i]) << " ";
  }
  std::cout << std::endl;

  GDALClose(dataset);
  VSIUnlink(vsi_mem_filename); 
  GDALDestroyDriverManager();

  return 0;
}