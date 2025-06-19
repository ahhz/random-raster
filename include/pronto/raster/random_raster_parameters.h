//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================
//

#pragma once

#include <ctime>
#include <functional> 
#include <limits>
#include <random>
#include <stdexcept>
#include <string> 
#include <type_traits> 
#include <vector>

#include <cpl_error.h>
#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_typetraits.h>

#include <nlohmann/json.hpp>

#include <pronto/raster/block_generator_interface.h>
#include <pronto/raster/random_block_generator.h>

namespace pronto {
  namespace raster {

    enum class distribution_type {
      uniform_integer,
      uniform_real,
      bernoulli,
      binomial,
      negative_binomial,
      geometric,
      weibull,
      extreme_value,
      cauchy,
      poisson,
      normal,
      exponential,
      gamma,
      lognormal,
      fisher_f,
      student_t,
      discrete_distribution,
      piecewise_constant,
      piecewise_linear,
      unspecified
    };

    // Forward declarations for functions assumed to be defined elsewhere
    // or to be implemented. Providing basic stubs if not defined in your snippets.
    std::string string_from_distribution_type(distribution_type dist_enum); 
    distribution_type string_to_distribution_type(const std::string& dist_str);
    std::string gdal_data_type_to_string(GDALDataType type);
    GDALDataType string_to_gdal_data_type(const std::string& str);
    bool is_random_raster_json(const nlohmann::json& j);

    struct random_raster_parameters
    {
      std::string  m_type;
      int          m_rows;
      int          m_cols;
      GDALDataType m_data_type;

      unsigned int m_seed;
      int m_block_rows;
      int m_block_cols;
      bool m_seed_set;
      bool m_block_rows_set;
      bool m_block_cols_set;

      distribution_type m_distribution;
      nlohmann::json m_distribution_parameters;

      random_raster_parameters();

      bool from_json(const nlohmann::json& j);
      nlohmann::json to_json() const;;
      bool validate() const;

      bool from_string_json(const std::string& json_string);

      template<typename T>
      T get_parameter(const std::string& key, T default_value, std::function<bool(T)> validate_func) const {
        T value;
        if (m_distribution_parameters.contains(key)) {
          try {
            value = m_distribution_parameters[key].get<T>();
          }
          catch (const nlohmann::json::type_error& e) {
            const std::string msg = "Type mismatch for parameter '" + key + "'. Expected " + typeid(T).name() + ", but JSON contains a different type: " + e.what();
            CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
            throw std::runtime_error(msg);
          }
          catch (const nlohmann::json::out_of_range& e) {
            const std::string msg = "Value out of range for parameter '" + key + "': " + e.what();
            CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
            throw std::runtime_error(msg);
          }
          catch (const nlohmann::json::exception& e) {
            // Catch other nlohmann::json exceptions for robust error handling
            const std::string msg = "JSON error retrieving parameter '" + key + "': " + e.what();
            CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
            throw std::runtime_error(msg);
          }
        }
        else {
          value = default_value;
        }

        if (!validate_func(value)) {
          const std::string msg = "Invalid value for parameter '" + key + "' based on validation function.";
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
        return value;
      }

      template<typename T>
      std::vector<T> get_vector_parameter(const std::string& key) const {
        if (!m_distribution_parameters.contains(key)) {
          const std::string msg = "Required parameter '" + key + "' for this distribution is missing.";
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
        const auto& j_array = m_distribution_parameters[key];
        if (!j_array.is_array()) {
          const std::string msg = "Parameter '" + key + "' must be a JSON array.";
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
        try {
          return j_array.get<std::vector<T>>(); // nlohmann::json can directly convert to std::vector<T>
        }
        catch (const nlohmann::json::type_error& e) {
          const std::string msg = "Type mismatch in array for parameter '" + key + "': " + e.what();
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
        catch (const nlohmann::json::out_of_range& e) {
          const std::string msg = "Value out of range in array for parameter '" + key + "': " + e.what();
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
        catch (const nlohmann::json::exception& e) {
          const std::string msg = "JSON error retrieving vector parameter '" + key + "': " + e.what();
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
      }

      template<class DistributionType, class GDALType=DistributionType::result_type>
      std::unique_ptr<block_generator_interface> make_generator(DistributionType dist) const
      {
        return std::unique_ptr<random_block_generator<DistributionType, GDALType>>(
          new random_block_generator<DistributionType, GDALType>(
            m_seed,
            m_rows,
            m_cols,
            m_block_rows_set ? m_block_rows : 256,
            m_block_cols_set ? m_block_cols : 256,
            dist
          )
        );
      }

      std::unique_ptr<block_generator_interface> create_block_generator() const {
        switch (m_data_type) {
        case GDT_Byte: return dispatch_integer_distribution<GDT_Byte>();
        case GDT_UInt16: return dispatch_integer_distribution<GDT_UInt16>();
        case GDT_Int16: return dispatch_integer_distribution<GDT_Int16>();
        case GDT_UInt32: return dispatch_integer_distribution<GDT_UInt32>();
        case GDT_Int32: return dispatch_integer_distribution<GDT_Int32>();
        case GDT_UInt64: return dispatch_integer_distribution<GDT_UInt64>();
        case GDT_Int64: return dispatch_integer_distribution<GDT_Int64>();
        case GDT_Float32: return dispatch_real_distribution<GDT_Float32>();
        case GDT_Float64: return dispatch_real_distribution<GDT_Float64>();
        case GDT_Unknown:
        default:
        {
          const std::string msg = "Unsupported or unknown GDALDataType for random raster generation: " + gdal_data_type_to_string(m_data_type);
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
        }
      }

      // This template handles all integer GDAL types 
      template<GDALDataType T>
      std::unique_ptr<block_generator_interface> dispatch_integer_distribution() const {
        using gdal_type = gdal::GDALDataTypeTraits<T>::type;
        using distribution_result_type = std::conditional_t<
          std::is_same_v<gdal_type, unsigned char>, // Compile-time check if gdal_target_type is unsigned char
          short,                                            // If true, use short
          gdal_type                                  // If false, use gdal_target_type itself
        >;
        switch (m_distribution) {
        case distribution_type::uniform_integer: {
          using dist_type =  std::uniform_int_distribution<distribution_result_type>;
          auto a = static_cast<distribution_result_type>(
            get_parameter<gdal_type>("a", std::numeric_limits<gdal_type>::min(), [](gdal_type) {return true; }));
          auto b = static_cast<distribution_result_type>(
            get_parameter<gdal_type>("b", std::numeric_limits<gdal_type>::max(), [&](gdal_type v) {return v >= a; }));
          
          return make_generator<dist_type, gdal_type>(dist_type(a, b));
        }
        case distribution_type::bernoulli: {
          using dist_type = std::bernoulli_distribution;
          auto p = get_parameter<double>("p", 0.5, [](double val) 
            { return val >= 0.0 && val <= 1.0; });
          return make_generator<dist_type, gdal_type>(dist_type(p));
        }
        case distribution_type::binomial: {
          using dist_type = std::binomial_distribution<distribution_result_type>;
          auto t = get_parameter<distribution_result_type>("t", 1, [&](distribution_result_type val)
            { return 0 <= val; });
          double p = get_parameter<double>("p", 0.5, [](double val) { return val >= 0.0 && val <= 1.0; });
          return make_generator<dist_type, gdal_type>(dist_type(t, p));
        }
                                        
        case distribution_type::negative_binomial: {
          using dist_type = std::negative_binomial_distribution<distribution_result_type>;
          auto k = get_parameter<distribution_result_type>("k", 1, [](distribution_result_type val) 
            { return val > 0; });
          double p = get_parameter<double>("p", 0.5, [](double val) { return val > 0.0 && val <= 1.0; });
          return make_generator<dist_type, gdal_type>(dist_type(k, p));
        }
        
        case distribution_type::geometric: {
          using dist_type = std::geometric_distribution<distribution_result_type>;
          double p = get_parameter<double>("p", 0.5, [](double val) { return val > 0.0 && val <= 1.0; });
          return make_generator<dist_type, gdal_type>(dist_type(p));
        }
        case distribution_type::poisson: {
          using dist_type = std::poisson_distribution<distribution_result_type>;
          double mean = get_parameter<double>("mean", 1.0, [](double val) { return val > 0.0; });
          return make_generator<dist_type, gdal_type>(dist_type(mean));
        }
        case distribution_type::discrete_distribution: {
          using dist_type = std::discrete_distribution<distribution_result_type>;
          std::vector<double> weights = get_vector_parameter<double>("weights");
          if (weights.empty()) {
            const std::string msg = "Discrete distribution requires 'weights' array with at least one element.";
            CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
            throw std::runtime_error(msg);
          }
          return make_generator<dist_type, gdal_type>(dist_type(weights.begin(), weights.end()));
        }
        default:
        {
          const std::string msg = "Unsupported integer distribution type for GDALDataType: " + gdal_data_type_to_string(T) + " and distribution: " + std::to_string(static_cast<int>(m_distribution));
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
        }
      }
      
      template<GDALDataType T>
      std::unique_ptr<block_generator_interface> dispatch_real_distribution() const {
        typedef typename gdal::GDALDataTypeTraits<T>::type RealType;

        switch (m_distribution) {
        case distribution_type::uniform_real: {
          typedef std::uniform_real_distribution<RealType> D;
          RealType a = get_parameter<RealType>("a", std::numeric_limits<RealType>::lowest(), [](RealType val) { return true; });
          RealType b = get_parameter<RealType>("b", std::numeric_limits<RealType>::max(), [&](RealType val) { return val >= a; });
          return make_generator(D(a, b));
        }
        case distribution_type::weibull: {
          typedef std::weibull_distribution<RealType> D;
          RealType a = get_parameter<RealType>("a", 1.0, [](RealType val) { return val > 0.0; });
          RealType b = get_parameter<RealType>("b", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator(D(a, b));
        }
        case distribution_type::extreme_value: {
          typedef std::extreme_value_distribution<RealType> D;
          RealType a = get_parameter<RealType>("a", 0.0, [](RealType val) { return true; });
          RealType b = get_parameter<RealType>("b", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator(D(a, b));
        }
        case distribution_type::cauchy: {
          typedef std::cauchy_distribution<RealType> D;
          RealType a = get_parameter<RealType>("a", 0.0, [](RealType val) { return true; });
          RealType b = get_parameter<RealType>("b", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator(D(a, b));
        }
        case distribution_type::normal: {
          typedef std::normal_distribution<RealType> D;
          RealType mean = get_parameter<RealType>("mean", 0.0, [](RealType val) { return true; });
          RealType stddev = get_parameter<RealType>("stddev", 1.0, [](RealType val) { return val >= 0.0; });
          return make_generator(D(mean, stddev));
        }
        case distribution_type::exponential: {
          typedef std::exponential_distribution<RealType> D;
          RealType lambda = get_parameter<RealType>("lambda", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator(D(lambda));
        }
        case distribution_type::gamma: {
          typedef std::gamma_distribution<RealType> D;
          RealType alpha = get_parameter<RealType>("alpha", 1.0, [](RealType val) { return val > 0.0; });
          RealType beta = get_parameter<RealType>("beta", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator(D(alpha, beta));
        }
        case distribution_type::lognormal: {
          typedef std::lognormal_distribution<RealType> D;
          RealType m = get_parameter<RealType>("m", 0.0, [](RealType val) { return true; });
          RealType s = get_parameter<RealType>("s", 1.0, [](RealType val) { return val >= 0.0; });
          return make_generator(D(m, s));
        }
        case distribution_type::fisher_f: {
          typedef std::fisher_f_distribution<RealType> D;
          RealType m = get_parameter<RealType>("m", 1.0, [](RealType val) { return val > 0.0; });
          RealType n = get_parameter<RealType>("n", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator(D(m, n));
        }
        case distribution_type::student_t: {
          typedef std::student_t_distribution<RealType> D;
          RealType n = get_parameter<RealType>("n", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator(D(n));
        }
        case distribution_type::piecewise_constant: {
          typedef std::piecewise_constant_distribution<RealType> D;
          std::vector<RealType> intervals = get_vector_parameter<RealType>("intervals");
          std::vector<RealType> densities = get_vector_parameter<RealType>("densities");
          if (intervals.size() < 2 || densities.size() != intervals.size() - 1) {
            const std::string msg = "Piecewise constant distribution requires at least two 'intervals' and 'densities' count one less than 'intervals'.";
            CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
            throw std::runtime_error(msg);
          }
          return make_generator(D(intervals.begin(), intervals.end(), densities.begin()));
        }
        case distribution_type::piecewise_linear: {
          typedef std::piecewise_linear_distribution<RealType> D;
          std::vector<RealType> intervals = get_vector_parameter<RealType>("intervals");
          std::vector<RealType> densities = get_vector_parameter<RealType>("densities");
          if (intervals.size() < 2 || densities.size() != intervals.size()) {
            const std::string msg = "Piecewise linear distribution requires at least two 'intervals' and 'densities' count equal to 'intervals'.";
            CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
            throw std::runtime_error(msg);
          }
          return make_generator(D(intervals.begin(), intervals.end(), densities.begin()));
        }
        default:
        {
          const std::string msg = "Unsupported real distribution type for GDALDataType: " + gdal_data_type_to_string(T) + " and distribution: " + std::to_string(static_cast<int>(m_distribution));
          CPLError(CE_Failure, CPLE_AppDefined, "%s", msg.c_str());
          throw std::runtime_error(msg);
        }
        }
      }

    };

  } // namespace raster
} // namespace pronto