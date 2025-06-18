//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================

#include <ctime>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>

#include <cpl_error.h>
#include <gdal.h>
#include <gdal_priv.h>

#include <nlohmann/json.hpp>

#include <pronto/raster/random_block_generator.h>
#include <pronto/raster/random_raster_parameters.h>

namespace pronto {
  namespace raster {
    // Helper function to convert string to distribution_type
    distribution_type string_to_distribution_type(const std::string& dist_str) {
      static const std::map<std::string, distribution_type> dist_map = {
        {"uniform_integer", distribution_type::uniform_integer},
        {"uniform_real", distribution_type::uniform_real},
        {"bernoulli", distribution_type::bernoulli},
        {"binomial", distribution_type::binomial},
        {"negative_binomial", distribution_type::negative_binomial},
        {"geometric", distribution_type::geometric},
        {"weibull", distribution_type::weibull},
        {"extreme_value", distribution_type::extreme_value},
        {"cauchy", distribution_type::cauchy},
        {"poisson", distribution_type::poisson},
        {"normal", distribution_type::normal},
        {"exponential", distribution_type::exponential},
        {"gamma", distribution_type::gamma},
        {"lognormal", distribution_type::lognormal},
        {"fisher_f", distribution_type::fisher_f},
        {"student_t", distribution_type::student_t},
        {"discrete_distribution", distribution_type::discrete_distribution},
        {"piecewise_constant", distribution_type::piecewise_constant},
        {"piecewise_linear", distribution_type::piecewise_linear}
      };

      std::map<std::string, distribution_type>::const_iterator it = dist_map.find(dist_str);
      if (it != dist_map.end()) {
        return it->second;
      }
      return distribution_type::unspecified;
    }

    std::string string_from_distribution_type(distribution_type dist_enum) {
      switch (dist_enum) {
      case distribution_type::uniform_integer: return "uniform_integer";
      case distribution_type::uniform_real: return "uniform_real";
      case distribution_type::bernoulli: return "bernoulli";
      case distribution_type::binomial: return "binomial";
      case distribution_type::negative_binomial: return "negative_binomial";
      case distribution_type::geometric: return "geometric";
      case distribution_type::weibull: return "weibull";
      case distribution_type::extreme_value: return "extreme_value";
      case distribution_type::cauchy: return "cauchy";
      case distribution_type::poisson: return "poisson";
      case distribution_type::normal: return "normal";
      case distribution_type::exponential: return "exponential";
      case distribution_type::gamma: return "gamma";
      case distribution_type::lognormal: return "lognormal";
      case distribution_type::fisher_f: return "fisher_f";
      case distribution_type::student_t: return "student_t";
      case distribution_type::discrete_distribution: return "discrete_distribution";
      case distribution_type::piecewise_constant: return "piecewise_constant";
      case distribution_type::piecewise_linear: return "piecewise_linear";
      case distribution_type::unspecified: return "unspecified"; // Should ideally not be serialized as a specific distribution type
      default: return "unknown_distribution"; // Fallback for any unhandled new types
      }
    }

    std::string gdal_data_type_to_string(GDALDataType type) {
      switch (type) {
      case GDT_Byte: return "GDT_Byte";
      case GDT_UInt16: return "GDT_UInt16";
      case GDT_Int16: return "GDT_Int16";
      case GDT_UInt32: return "GDT_UInt32";
      case GDT_Int32: return "GDT_Int32";
      case GDT_Float32: return "GDT_Float32";
      case GDT_Float64: return "GDT_Float64";
      default: return "GDT_Unknown";
      }
    }

    GDALDataType string_to_gdal_data_type(const std::string& str) {
      if (str == "GDT_Byte") return GDT_Byte;
      else if (str == "GDT_UInt16") return GDT_UInt16;
      else if (str == "GDT_Int16") return GDT_Int16;
      else if (str == "GDT_UInt32") return GDT_UInt32;
      else if (str == "GDT_Int32") return GDT_Int32;
      else if (str == "GDT_Float32") return GDT_Float32;
      else if (str == "GDT_Float64") return GDT_Float64;
      else return GDT_Unknown;
    }

    bool is_random_raster_json(const nlohmann::json& j) {
      try {
        // Check if the "type" field exists and equals "RANDOM_RASTER".
        return j.at("type").get<std::string>() == "RANDOM_RASTER";
      }
      catch (const nlohmann::json::exception&) {
        // If "type" field doesn't exist, it's not our JSON.
        return false;
      }
    }
    bool random_raster_parameters::from_json(const nlohmann::json& j) {
      try {
        m_type = j.at("type").get<std::string>();

        // Mandatory fields - will throw if not present
        j.at("rows").get_to(m_rows);
        j.at("cols").get_to(m_cols);
        j.at("bits").get_to(m_bits);

        std::string data_type_str;
        j.at("data_type").get_to(data_type_str);
        m_data_type = string_to_gdal_data_type(data_type_str);

        // Optional parameters
        if (j.contains("seed") && j["seed"].is_number_integer()) {
          m_seed = j["seed"].get<unsigned int>();
					m_seed_set = true; // Update flag to indicate seed was set
        }
               
        if (j.contains("block_rows") && j["block_rows"].is_number_integer()) {
          m_block_rows = j["block_rows"].get<int>();
          m_block_rows_set = true; // Update flag
        }

        if (j.contains("block_cols") && j["block_cols"].is_number_integer()) {
          m_block_cols = j["block_cols"].get<int>();
          m_block_cols_set = true; // Update flag
        }

        // Distribution type
        if (j.contains("distribution")) {
          std::string dist_str = j.at("distribution").get<std::string>();
          m_distribution = string_to_distribution_type(dist_str);
        }
        else {
          m_distribution = distribution_type::unspecified;
        }

        // Distribution parameters
        m_distribution_parameters.clear(); // Clear any existing parameters
        if (j.contains("parameters") && j["parameters"].is_object()) {
          const auto& params_json = j["parameters"];
          for (auto& [key, value] : params_json.items()) {
            // Store values as their JSON string representation.
            // This handles numbers, booleans, arrays, etc., correctly.
            m_distribution_parameters[key] = value.dump();
          }
        }
        // Perform internal validation.
        return validate();
      }
      catch (const nlohmann::json::exception& e) {
        CPLError(CE_Failure, CPLE_AppDefined, "Error parsing JSON parameters: %s. Check for required fields: 'type', 'width', 'height', 'bits', 'data_type'.", e.what());
        return false;
      }
      catch (const std::exception& e) {
        CPLError(CE_Failure, CPLE_AppDefined, "Error processing JSON parameters: %s", e.what());
        return false;
      }
    }

    nlohmann::json random_raster_parameters::to_json() const {
      nlohmann::json j;
      j["type"] = m_type;
      j["rows"] = m_rows;
      j["cols"] = m_cols;
      j["bits"] = m_bits;
      j["data_type"] = gdal_data_type_to_string(m_data_type);

      // Only include seed, block_rows, block_cols if it was set by user
      if (m_seed_set) {
        j["seed"] = m_seed;
      }
      if (m_block_rows_set) {
        j["block_rows"] = m_block_rows;
      }
      if (m_block_cols_set) {
         j["block_cols"] = m_block_cols;
      }

      // Include distribution type
      if (m_distribution != distribution_type::unspecified) {
        j["distribution"] = string_from_distribution_type(m_distribution);
      }

      // Include distribution parameters
      if (!m_distribution_parameters.empty()) {
        nlohmann::json params_json;
        for (const auto& pair : m_distribution_parameters) {
          // Parse the stored string back to a JSON value for correct serialization
          params_json[pair.first] = nlohmann::json::parse(pair.second);
        }
        j["parameters"] = params_json;
      }

      return j;
    }
    
    bool random_raster_parameters::validate() const {
      if (m_type != "RANDOM_RASTER") {
        CPLError(CE_Failure, CPLE_AppDefined, "Invalid 'type' parameter. Expected 'RANDOM_RASTER', got '%s'.", m_type.c_str());
        return false;
      }
      if (m_rows <= 0 || m_cols <= 0) {
        CPLError(CE_Failure, CPLE_AppDefined, "Random raster dimensions must be positive (rows x cols provided: %dx%d).", m_rows, m_cols);
        return false;
      }
      if (m_data_type == GDT_Unknown) {
        CPLError(CE_Failure, CPLE_AppDefined, "Data type must be specified and valid.");
        return false;
      }
      if (m_bits <= 0 || m_bits > GDALGetDataTypeSizeBits(m_data_type)) {
        CPLError(CE_Failure, CPLE_AppDefined, "Invalid number of bits (%d) for data type %s (max: %d).", m_bits, GDALGetDataTypeName(m_data_type), GDALGetDataTypeSizeBits(m_data_type));
        return false;
      }
      if (m_block_rows <= 0 || m_block_cols <= 0) {
        CPLError(CE_Failure, CPLE_AppDefined, "Block sizes must be positive (BlockRows=%d, BlockCols=%d).", m_block_rows, m_block_cols);
        return false;
      }
      return true;
    }

    bool random_raster_parameters::from_string_json(const std::string& json_string)
    {
      *this = random_raster_parameters(); // Reset all members to defaults

      try {
        nlohmann::json j = nlohmann::json::parse(json_string);

        // Check if it's the correct type of JSON for our driver.
        if (!is_random_raster_json(j)) {
          return false; // Quiet failure for incorrect type
        }
        // Proceed with full population and validation.
        return from_json(j);
      }
      catch (const nlohmann::json::exception&) {
        return false; // Quiet failure for malformed JSON
      }
    }
  } // namespace raster
} // namespace pronto