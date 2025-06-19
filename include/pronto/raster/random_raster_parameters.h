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
#include <functional> // For std::function
#include <random>
#include <limits>
#include <stdexcept>
#include <type_traits> // For std::is_integral, std::is_floating_point
#include <vector>
#include <string> // Added for std::string functions

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
            throw std::runtime_error("Type mismatch for parameter '" + key + "'. Expected " + typeid(T).name() + ", but JSON contains a different type: " + e.what());
          }
          catch (const nlohmann::json::out_of_range& e) {
            throw std::runtime_error("Value out of range for parameter '" + key + "': " + e.what());
          }
          catch (const nlohmann::json::exception& e) {
            // Catch other nlohmann::json exceptions for robust error handling
            throw std::runtime_error("JSON error retrieving parameter '" + key + "': " + e.what());
          }
        }
        else {
          value = default_value;
        }

        if (!validate_func(value)) {
          throw std::runtime_error("Invalid value for parameter '" + key + "' based on validation function.");
        }
        return value;
      }

      template<typename T>
      std::vector<T> get_vector_parameter(const std::string& key) const {
        if (!m_distribution_parameters.contains(key)) {
          throw std::runtime_error("Required parameter '" + key + "' for this distribution is missing.");
        }
        const auto& j_array = m_distribution_parameters[key];
        if (!j_array.is_array()) {
          throw std::runtime_error("Parameter '" + key + "' must be a JSON array.");
        }
        try {
          return j_array.get<std::vector<T>>(); // nlohmann::json can directly convert to std::vector<T>
        }
        catch (const nlohmann::json::type_error& e) {
          throw std::runtime_error("Type mismatch in array for parameter '" + key + "': " + e.what());
        }
        catch (const nlohmann::json::out_of_range& e) {
          throw std::runtime_error("Value out of range in array for parameter '" + key + "': " + e.what());
        }
        catch (const nlohmann::json::exception& e) {
          throw std::runtime_error("JSON error retrieving vector parameter '" + key + "': " + e.what());
        }
      }

      // make_generator takes DistributionType (e.g., std::uniform_int_distribution<short>)
      // and TargetGdalTypeEnum (e.g., GDT_Byte).
      // It is expected that random_block_generator handles the safe conversion/casting
      // from DistributionType::result_type to TargetGdalType.
      template<class DistributionType, GDALDataType TargetGdalTypeEnum>
      std::unique_ptr<block_generator_interface> make_generator(DistributionType dist) const
      {
        typedef typename gdal::GDALDataTypeTraits<TargetGdalTypeEnum>::type TargetGdalType;
        return std::unique_ptr<random_block_generator<DistributionType, TargetGdalType>>(
          new random_block_generator<DistributionType, TargetGdalType>(
            m_seed,
            m_rows,
            m_cols,
            m_block_rows_set ? m_block_rows : 256,
            m_block_cols_set ? m_block_cols : 256,
            dist
          )
        );
      }

      // Default specialized dispatch, to be caught by static_assert if not defined.
      template<GDALDataType T, distribution_type D>
      std::unique_ptr<block_generator_interface> dispatch() const
      {
        static_assert(static_cast<int>(D) == -1, "Unsupported distribution for this GDALDataType combination.");
        return std::unique_ptr<block_generator_interface>();
      }

      // --- Implementation of general dispatch for integer distributions (for non-GDT_Byte types) ---
      template<GDALDataType T>
      std::unique_ptr<block_generator_interface> dispatch_integer_distribution() const {
        // This general template handles all integer GDAL types *except* GDT_Byte for uniform_integer,
        // where GDT_Byte has its own specialization above.
        // It also handles all other integer distributions for all IntType which are suitable for std::..._distribution.
        typedef typename gdal::GDALDataTypeTraits<T>::type IntType;

        switch (m_distribution) {
        case distribution_type::uniform_integer: {
          typedef std::uniform_int_distribution<IntType> D; // IntType is suitable here (e.g., GUInt16, GInt32)

          IntType default_a = std::numeric_limits<IntType>::min();
          IntType default_b = std::numeric_limits<IntType>::max();
          std::function<bool(IntType)> current_range_validator = [&](IntType val) {
            return val >= default_a && val <= default_b;
            };

          IntType a = get_parameter<IntType>("a", default_a, current_range_validator);
          IntType b = get_parameter<IntType>("b", default_b, current_range_validator);

          if (!(a <= b)) {
            throw std::runtime_error("Invalid parameters for uniform_int_distribution: 'a' must be less than or equal to 'b'.");
          }
          return make_generator<D, T>(D(a, b));
        }
        case distribution_type::bernoulli: {
          typedef std::bernoulli_distribution D;
          double p = get_parameter<double>("p", 0.5, [](double val) { return val >= 0.0 && val <= 1.0; });
          return make_generator<D, T>(D(p));
        }
        case distribution_type::binomial: {
          typedef std::binomial_distribution<IntType> D;
          IntType t = get_parameter<IntType>("t", 1, [](IntType val) { return val >= 0; });
          double p = get_parameter<double>("p", 0.5, [](double val) { return val >= 0.0 && val <= 1.0; });
          return make_generator<D, T>(D(t, p));
        }
        case distribution_type::negative_binomial: {
          typedef std::negative_binomial_distribution<IntType> D;
          IntType k = get_parameter<IntType>("k", 1, [](IntType val) { return val > 0; });
          double p = get_parameter<double>("p", 0.5, [](double val) { return val > 0.0 && val <= 1.0; });
          return make_generator<D, T>(D(k, p));
        }
        case distribution_type::geometric: {
          typedef std::geometric_distribution<IntType> D;
          double p = get_parameter<double>("p", 0.5, [](double val) { return val > 0.0 && val <= 1.0; });
          return make_generator<D, T>(D(p));
        }
        case distribution_type::poisson: {
          typedef std::poisson_distribution<IntType> D;
          double mean = get_parameter<double>("mean", 1.0, [](double val) { return val > 0.0; });
          return make_generator<D, T>(D(mean));
        }
        case distribution_type::discrete_distribution: {
          typedef std::discrete_distribution<IntType> D;
          std::vector<double> weights = get_vector_parameter<double>("weights");
          if (weights.empty()) {
            throw std::runtime_error("Discrete distribution requires 'weights' array with at least one element.");
          }
          return make_generator<D, T>(D(weights.begin(), weights.end()));
        }
        default:
          throw std::runtime_error("Unsupported integer distribution type for GDALDataType: " + gdal_data_type_to_string(T) + " and distribution: " + std::to_string(static_cast<int>(m_distribution)));
        }
      }
      // --- Specialization for GDT_Byte integer distributions ---
      // This specialization ensures that `std::uniform_int_distribution` and other integer distributions
      // use a standard integer type (like short or int) to avoid undefined behavior,
      // while still producing GDT_Byte (unsigned char) output.
      template<>
      std::unique_ptr<block_generator_interface> dispatch_integer_distribution<GDT_Byte>() const {
        // GByte is unsigned char.
        // std::uniform_int_distribution and others cannot take char/unsigned char directly.
        // We will use 'short' or 'int' as the internal distribution type and ensure values are 0-255.
        // random_block_generator for GDT_Byte will then safely cast from short/int to GByte.
        switch (m_distribution) {
        case distribution_type::uniform_integer: {
          // Use short for the internal distribution type to avoid UB with unsigned char.
          typedef std::uniform_int_distribution<short> D;

          // Default parameters for GDT_Byte uniform distribution (0-255)
          short default_a = 0;
          short default_b = 255;

          // Validation lambda to ensure input 'a' and 'b' are within 0-255 range.
          std::function<bool(short)> byte_range_validator = [](short val) {
            return val >= 0 && val <= 255;
            };

          // Get parameters as 'short' and validate against 0-255 range.
          short a = get_parameter<short>("a", default_a, byte_range_validator);
          short b = get_parameter<short>("b", default_b, byte_range_validator);

          if (!(a <= b)) {
            throw std::runtime_error("Invalid parameters for uniform_int_distribution: 'a' must be less than or equal to 'b'.");
          }
          // Call make_generator with the internal distribution type (short)
          // and the target GDAL output type (GDT_Byte).
          return make_generator<D, GDT_Byte>(D(a, b));
        }
        case distribution_type::bernoulli: {
          // Bernoulli produces bool, which safely converts to GByte (0 or 1).
          typedef std::bernoulli_distribution D;
          double p = get_parameter<double>("p", 0.5, [](double val) { return val >= 0.0 && val <= 1.0; });
          return make_generator<D, GDT_Byte>(D(p));
        }
        case distribution_type::binomial: {
          // Use int for binomial distribution's IntType.
          typedef std::binomial_distribution<int> D;
          int t = get_parameter<int>("t", 1, [](int val) { return val >= 0 && val <= 255; }); // Ensure trials can't exceed GByte max
          double p = get_parameter<double>("p", 0.5, [](double val) { return val >= 0.0 && val <= 1.0; });
          return make_generator<D, GDT_Byte>(D(t, p));
        }
        case distribution_type::negative_binomial: {
          // Use int for negative_binomial distribution's IntType.
          typedef std::negative_binomial_distribution<int> D;
          int k = get_parameter<int>("k", 1, [](int val) { return val > 0; });
          double p = get_parameter<double>("p", 0.5, [](double val) { return val > 0.0 && val <= 1.0; });
          return make_generator<D, GDT_Byte>(D(k, p));
        }
        case distribution_type::geometric: {
          // Use int for geometric distribution's IntType.
          typedef std::geometric_distribution<int> D;
          double p = get_parameter<double>("p", 0.5, [](double val) { return val > 0.0 && val <= 1.0; });
          return make_generator<D, GDT_Byte>(D(p));
        }
        case distribution_type::poisson: {
          // Use int for poisson distribution's IntType.
          typedef std::poisson_distribution<int> D;
          double mean = get_parameter<double>("mean", 1.0, [](double val) { return val > 0.0; });
          // Optional: add validation for mean to prevent results far exceeding 255 if needed for practical use.
          // E.g., [](double val) { return val > 0.0 && val < 100.0; } // Arbitrary upper limit
          return make_generator<D, GDT_Byte>(D(mean));
        }
        case distribution_type::discrete_distribution: {
          // Use int for discrete_distribution's IntType.
          typedef std::discrete_distribution<int> D;
          std::vector<double> weights = get_vector_parameter<double>("weights");
          if (weights.empty()) {
            throw std::runtime_error("Discrete distribution requires 'weights' array with at least one element.");
          }
          return make_generator<D, GDT_Byte>(D(weights.begin(), weights.end()));
        }
        default:
          throw std::runtime_error("Unsupported integer distribution type for GDT_Byte and distribution: " + std::to_string(static_cast<int>(m_distribution)));
        }
      }

      template<GDALDataType T>
      std::unique_ptr<block_generator_interface> dispatch_real_distribution() const {
        typedef typename gdal::GDALDataTypeTraits<T>::type RealType;

        switch (m_distribution) {
        case distribution_type::uniform_real: {
          typedef std::uniform_real_distribution<RealType> D;
          RealType a = get_parameter<RealType>("a", std::numeric_limits<RealType>::lowest(), [](RealType val) { return true; });
          RealType b = get_parameter<RealType>("b", std::numeric_limits<RealType>::max(), [](RealType val) { return true; });

          if (!(a <= b)) {
            throw std::runtime_error("Invalid parameters for uniform_real_distribution: 'a' must be less than or equal to 'b'.");
          }
          return make_generator<D, T>(D(a, b));
        }
        case distribution_type::weibull: {
          typedef std::weibull_distribution<RealType> D;
          RealType a = get_parameter<RealType>("a", 1.0, [](RealType val) { return val > 0.0; });
          RealType b = get_parameter<RealType>("b", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator<D, T>(D(a, b));
        }
        case distribution_type::extreme_value: {
          typedef std::extreme_value_distribution<RealType> D;
          RealType a = get_parameter<RealType>("a", 0.0, [](RealType val) { return true; });
          RealType b = get_parameter<RealType>("b", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator<D, T>(D(a, b));
        }
        case distribution_type::cauchy: {
          typedef std::cauchy_distribution<RealType> D;
          RealType a = get_parameter<RealType>("a", 0.0, [](RealType val) { return true; });
          RealType b = get_parameter<RealType>("b", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator<D, T>(D(a, b));
        }
        case distribution_type::normal: {
          typedef std::normal_distribution<RealType> D;
          RealType mean = get_parameter<RealType>("mean", 0.0, [](RealType val) { return true; });
          RealType stddev = get_parameter<RealType>("stddev", 1.0, [](RealType val) { return val >= 0.0; });
          return make_generator<D, T>(D(mean, stddev));
        }
        case distribution_type::exponential: {
          typedef std::exponential_distribution<RealType> D;
          RealType lambda = get_parameter<RealType>("lambda", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator<D, T>(D(lambda));
        }
        case distribution_type::gamma: {
          typedef std::gamma_distribution<RealType> D;
          RealType alpha = get_parameter<RealType>("alpha", 1.0, [](RealType val) { return val > 0.0; });
          RealType beta = get_parameter<RealType>("beta", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator<D, T>(D(alpha, beta));
        }
        case distribution_type::lognormal: {
          typedef std::lognormal_distribution<RealType> D;
          RealType m = get_parameter<RealType>("m", 0.0, [](RealType val) { return true; });
          RealType s = get_parameter<RealType>("s", 1.0, [](RealType val) { return val >= 0.0; });
          return make_generator<D, T>(D(m, s));
        }
        case distribution_type::fisher_f: {
          typedef std::fisher_f_distribution<RealType> D;
          RealType m = get_parameter<RealType>("m", 1.0, [](RealType val) { return val > 0.0; });
          RealType n = get_parameter<RealType>("n", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator<D, T>(D(m, n));
        }
        case distribution_type::student_t: {
          typedef std::student_t_distribution<RealType> D;
          RealType n = get_parameter<RealType>("n", 1.0, [](RealType val) { return val > 0.0; });
          return make_generator<D, T>(D(n));
        }
        case distribution_type::piecewise_constant: {
          typedef std::piecewise_constant_distribution<RealType> D;
          std::vector<RealType> intervals = get_vector_parameter<RealType>("intervals");
          std::vector<RealType> densities = get_vector_parameter<RealType>("densities");
          if (intervals.size() < 2 || densities.size() != intervals.size() - 1) {
            throw std::runtime_error("Piecewise constant distribution requires at least two 'intervals' and 'densities' count one less than 'intervals'.");
          }
          return make_generator<D, T>(D(intervals.begin(), intervals.end(), densities.begin()));
        }
        case distribution_type::piecewise_linear: {
          typedef std::piecewise_linear_distribution<RealType> D;
          std::vector<RealType> intervals = get_vector_parameter<RealType>("intervals");
          std::vector<RealType> densities = get_vector_parameter<RealType>("densities");
          if (intervals.size() < 2 || densities.size() != intervals.size()) {
            throw std::runtime_error("Piecewise linear distribution requires at least two 'intervals' and 'densities' count equal to 'intervals'.");
          }
          return make_generator<D, T>(D(intervals.begin(), intervals.end(), densities.begin()));
        }
        default:
          throw std::runtime_error("Unsupported real distribution type for GDALDataType: " + gdal_data_type_to_string(T) + " and distribution: " + std::to_string(static_cast<int>(m_distribution)));
        }
      }

      std::unique_ptr<block_generator_interface> create_block_generator() const {
        switch (m_data_type) {
        case GDT_Byte: return dispatch_integer_distribution<GDT_Byte>();
        case GDT_UInt16: return dispatch_integer_distribution<GDT_UInt16>();
        case GDT_Int16: return dispatch_integer_distribution<GDT_Int16>();
        case GDT_UInt32: return dispatch_integer_distribution<GDT_UInt32>();
        case GDT_Int32: return dispatch_integer_distribution<GDT_Int32>();
        case GDT_Float32: return dispatch_real_distribution<GDT_Float32>();
        case GDT_Float64: return dispatch_real_distribution<GDT_Float64>();
        case GDT_Unknown:
        default:
          throw std::runtime_error("Unsupported or unknown GDALDataType for random raster generation: " + gdal_data_type_to_string(m_data_type));
        }
      }
    };

  } // namespace raster
} // namespace pronto