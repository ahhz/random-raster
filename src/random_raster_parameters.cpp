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

     // This is the list of datatypes suppred by the RANDOM_RASTER format
    static const std::vector<GDALDataType> s_supportedGDTs = {
        GDT_Byte,
        GDT_UInt16,
        GDT_Int16,
        GDT_UInt32,
        GDT_Int32,
        GDT_UInt64,
        GDT_Int64,
        GDT_Float32,
        GDT_Float64,
        // Add other GDALDataTypes here as needed for your application
        // e.g., GDT_CInt16, GDT_CFloat32, etc.
    };

    std::string gdal_data_type_to_string(GDALDataType type) 
    {
      bool is_supported = std::find(s_supportedGDTs.begin(), s_supportedGDTs.end(), type) != s_supportedGDTs.end();
      if (is_supported) {
        return GDALGetDataTypeName(type);
      }
      else {
        return "Unknown";
      }
    }
        
    GDALDataType string_to_gdal_data_type(const std::string& str) {
      static std::map<std::string, GDALDataType> s_stringToGDTMap;
      static std::once_flag s_mapInitFlag;
      std::call_once(s_mapInitFlag, []() {
        for (GDALDataType type : s_supportedGDTs) {
          s_stringToGDTMap[GDALGetDataTypeName(type)] = type;
        }
        });

      // Attempt to find the input string in our populated map.
      auto it = s_stringToGDTMap.find(str);
      if (it != s_stringToGDTMap.end()) {
        return it->second;
      }
      else {
        // If the string is not supported
        return GDT_Unknown;
      }
    }
       
    bool is_random_raster_json(const nlohmann::json& j) {
      return j.contains("type") && j["type"].is_string() && j["type"].get<std::string>() == "RANDOM_RASTER";
    }

    random_raster_parameters::random_raster_parameters() :
      m_type("RANDOM_RASTER"),
      m_rows(0),
      m_cols(0),
      m_data_type(GDT_Unknown),
      m_seed(static_cast<unsigned int>(std::time(nullptr))),
      m_block_rows(256),
      m_block_cols(256),
      m_seed_set(false),
      m_block_rows_set(false),
      m_block_cols_set(false),
      m_distribution(distribution_type::unspecified),
      m_distribution_parameters(nlohmann::json::object()) // Initialize as empty JSON object
    {
    }
    bool random_raster_parameters::from_json(const nlohmann::json & j) 
    {
      try {
        m_type = j.value("type", "RANDOM_RASTER");
        m_rows = j.value("rows", 0);
        m_cols = j.value("cols", 0);
    
        if (j.contains("data_type") && j["data_type"].is_string()) {
          m_data_type = string_to_gdal_data_type(j["data_type"].get<std::string>());
        }
        else {
          m_data_type = GDT_Unknown; // Default or error handling
        }

        if (j.contains("seed")) {
          m_seed = j["seed"].get<unsigned int>();
          m_seed_set = true;
        }

        if (j.contains("block_rows")) {
          m_block_rows = j["block_rows"].get<int>();
          m_block_rows_set = true;
        }

        if (j.contains("block_cols")) {
          m_block_cols = j["block_cols"].get<int>();
          m_block_cols_set = true;
        }

        if (j.contains("distribution") && j["distribution"].is_string()) {
          m_distribution = string_to_distribution_type(j["distribution"].get<std::string>());
        }
        else {
          m_distribution = distribution_type::unspecified; // Default or error
        }

        if (j.contains("distribution_parameters") && j["distribution_parameters"].is_object()) {
          m_distribution_parameters = j["distribution_parameters"];
        }
        else {
          m_distribution_parameters = nlohmann::json::object(); // Ensure it's an empty object if not present
        }

      }
      catch (const nlohmann::json::exception& e) {
        CPLError(CE_Failure, CPLE_AppDefined, "JSON parsing error: %s", e.what());
        return false;
      }
      catch (const std::runtime_error& e) {
        CPLError(CE_Failure, CPLE_AppDefined, "Invalid parameters: %s", e.what());
      }
      return validate(); // will also raise errors
    }


    nlohmann::json random_raster_parameters::to_json() const {
      nlohmann::json j;
      j["type"] = m_type;
      j["rows"] = m_rows;
      j["cols"] = m_cols;
      j["data_type"] = gdal_data_type_to_string(m_data_type);
      if (m_seed_set) {
        j["seed"] = m_seed;
      }
      if (m_block_rows_set) {
        j["block_rows"] = m_block_rows;
      }
      if (m_block_cols_set) {
        j["block_cols"] = m_block_cols;
      }
      j["distribution"] = string_from_distribution_type(m_distribution);
      j["distribution_parameters"] = m_distribution_parameters; // Directly use the stored JSON object
      return j;
    }
    
    bool random_raster_parameters::validate() const {
      if (m_type != "RANDOM_RASTER") {
        CPLError(CE_Failure, CPLE_AppDefined, "Invalid type, must be RANDOM_RASTER.");
        return false;
      }
      if (m_rows <= 0 || m_cols <= 0) {
        CPLError(CE_Failure, CPLE_AppDefined, "Rows and cols must be greater than 0.");
        return false;
      }
      if (m_data_type == GDT_Unknown) {
        CPLError(CE_Failure, CPLE_AppDefined, "Data type not specified or unknown.");
        return false;
      }
      if (m_distribution == distribution_type::unspecified) {
        CPLError(CE_Failure, CPLE_AppDefined, "Distribution type not specified or unknown.");
        return false;
      }
      // Add more specific validation for distribution parameters if needed,
      // perhaps by trying to get them with validation in a dummy call.
      return true;
    }

    bool random_raster_parameters::from_string_json(const std::string& json_string)
    {
      try {
        nlohmann::json j = nlohmann::json::parse(json_string);
        return from_json(j);
      }
      catch (const nlohmann::json::parse_error& e) {
        CPLError(CE_Failure, CPLE_AppDefined, "JSON parse error from string: %s", e.what());
        return false;
      }
    }
  } // namespace raster
} // namespace pronto