#pragma once
#include <gdal_priv.h>
#include <cpl_vsi.h> // For VSI file operations
#include <nlohmann/json.hpp> // For JSON parsing
#include <pronto/raster/block_generator_interface.h>
#include <pronto/raster/random_block_generator.h>
#include <pronto/raster/random_dataset.h>

#include <random>

namespace pronto {
namespace raster {
	
struct random_raster_parameters
{
  std::string  m_type = "RANDOM_RASTER"; // Mandatory field to identify this driver's JSON
  int          m_width = 0;
  int          m_height = 0;
  int          m_bits = 0;
  GDALDataType m_data_type = GDT_Unknown;
  unsigned int m_seed = static_cast<unsigned int>(std::time(nullptr)); // Default seed
  int          m_block_x_size = 256; // Default block width
  int          m_block_y_size = 256; // Default block height

  static std::string gdal_data_type_to_string(GDALDataType type);
  static GDALDataType string_to_gdal_data_type(const std::string& str);
  static bool is_random_raster_json(const nlohmann::json& j);

  bool from_json(const nlohmann::json& j);
  nlohmann::json to_json() const;
  bool validate() const;
  bool from_string_json(const std::string& json_string);
  std::unique_ptr<block_generator_interface> create_block_generator() const;
};

} // namespace raster
} // namespace pronto