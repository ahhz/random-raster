
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <random>
#include <functional>
#include <limits>
#include <chrono> // For std::chrono::system_clock
#include <algorithm>
#include <cstdint>

#include <nlohmann/json.hpp>

#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_typetraits.h>

#include <pronto/raster/block_generator_interface.h>
#include <pronto/raster/random_block_generator.h> 
#include <pronto/raster/random_raster_dataset.h> 
namespace pronto {
  namespace raster {
// An enum to represent all supported distributions
    enum class distribution_type {
      unspecified,
      // Integer Distributions
      uniform_integer,
      bernoulli,
      binomial,
      negative_binomial,
      geometric,
      poisson,
      // Real Distributions
      uniform_real,
      normal,
      lognormal,
      gamma,
      exponential,
      weibull,
      extreme_value,
      cauchy,
      fisher_f,
      student_t,
      chi_squared,
      // Sampling Distributions
      discrete,
      piecewise_constant,
      piecewise_linear
    };

    // Helper function to convert a string to our distribution_type enum
    distribution_type string_to_distribution_type(const std::string& dist_str) {
      static const std::map<std::string, distribution_type> dist_map = {
        {"uniform_integer", distribution_type::uniform_integer},
        {"bernoulli", distribution_type::bernoulli},
        {"binomial", distribution_type::binomial},
        {"negative_binomial", distribution_type::negative_binomial},
        {"geometric", distribution_type::geometric},
        {"poisson", distribution_type::poisson},
        {"uniform_real", distribution_type::uniform_real},
        {"normal", distribution_type::normal},
        {"lognormal", distribution_type::lognormal},
        {"gamma", distribution_type::gamma},
        {"exponential", distribution_type::exponential},
        {"weibull", distribution_type::weibull},
        {"extreme_value", distribution_type::extreme_value},
        {"cauchy", distribution_type::cauchy},
        {"fisher_f", distribution_type::fisher_f},
        {"student_t", distribution_type::student_t},
        {"chi_squared", distribution_type::chi_squared},
        {"discrete", distribution_type::discrete},
        {"piecewise_constant", distribution_type::piecewise_constant},
        {"piecewise_linear", distribution_type::piecewise_linear}
      };

      auto it = dist_map.find(dist_str);
      if (it != dist_map.end()) {
        return it->second;
      }
      else {
        throw std::runtime_error(dist_str + " is not a supported distribution type");
      }
    }

    // Helper to convert distribution_type to string for error messages
    std::string to_string(distribution_type dt) {
      static const std::map<distribution_type, std::string> dist_map = {
        {distribution_type::uniform_integer, "uniform_integer"},
        {distribution_type::bernoulli, "bernoulli"},
        {distribution_type::binomial, "binomial"},
        {distribution_type::negative_binomial, "negative_binomial"},
        {distribution_type::geometric, "geometric"},
        {distribution_type::poisson, "poisson"},
        {distribution_type::uniform_real, "uniform_real"},
        {distribution_type::normal, "normal"},
        {distribution_type::lognormal, "lognormal"},
        {distribution_type::gamma, "gamma"},
        {distribution_type::exponential, "exponential"},
        {distribution_type::weibull, "weibull"},
        {distribution_type::extreme_value, "extreme_value"},
        {distribution_type::cauchy, "cauchy"},
        {distribution_type::fisher_f, "fisher_f"},
        {distribution_type::student_t, "student_t"},
        {distribution_type::chi_squared, "chi_squared"},
        {distribution_type::discrete, "discrete"},
        {distribution_type::piecewise_constant, "piecewise_constant"},
        {distribution_type::piecewise_linear, "piecewise_linear"}
      };
      auto it = dist_map.find(dt);
      if (it != dist_map.end()) {
        return it->second;
      }
      return "unspecified_or_unknown_distribution";
    }

    template <typename ValueType>
    ValueType get_required_param_no_bounds(
      const nlohmann::json& j,
      const std::string& key)
    {
      if (!j.contains(key)) {
        throw std::runtime_error("Missing required parameter: '" + key + "'");
      }
      try {
        return j.at(key).get<ValueType>();
      }
      catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Type error or out of range for parameter '" + key + "': " + e.what());
      }
    }

    template <typename ValueType>
    ValueType get_required_param(
      const nlohmann::json& j,
      const std::string& key,
      const std::pair<ValueType, bool>& min_bound = { std::numeric_limits<ValueType>::lowest(), true },
      const std::pair<ValueType, bool>& max_bound = { std::numeric_limits<ValueType>::max(), true })
    {
      if (!j.contains(key)) {
        throw std::runtime_error("Missing required parameter: '" + key + "'");
      }
      ValueType value = get_required_param_no_bounds<ValueType>(j, key);

      // Minimum boundary check
      if (min_bound.second) { // Inclusive check
        if (value < min_bound.first) {
          throw std::runtime_error("Parameter '" + key + "' with value " + std::to_string(value) +
            " is below the minimum inclusive bound of " + std::to_string(min_bound.first));
        }
      }
      else { // Exclusive check
        if (value <= min_bound.first) {
          throw std::runtime_error("Parameter '" + key + "' with value " + std::to_string(value) +
            " must be greater than the minimum exclusive bound of " + std::to_string(min_bound.first));
        }
      }
      // Maximum boundary check
      if (max_bound.second) { // Inclusive check
        if (value > max_bound.first) {
          throw std::runtime_error("Parameter '" + key + "' with value " + std::to_string(value) +
            " is above the maximum inclusive bound of " + std::to_string(max_bound.first));
        }
      }
      else { // Exclusive check
        if (value >= max_bound.first) {
          throw std::runtime_error("Parameter '" + key + "' with value " + std::to_string(value) +
            " must be less than the maximum exclusive bound of " + std::to_string(max_bound.first));
        }
      }

      return value;
    }
    // Overload for vector types where min/max bounds are not applicable
    template <typename Vector, typename ElementType = Vector::value_type>
    std::vector<ElementType> get_required_param_vector(const nlohmann::json& j, const std::string& key) {
      if (!j.contains(key)) {
        throw std::runtime_error("Missing required parameter: '" + key + "'");
      }
      try {
        return j.at(key).get<std::vector<ElementType>>();
      }
      catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Type error for parameter '" + key + "': " + e.what());
      }
    }

    template <typename ValueType>
    ValueType get_optional_param(
      const nlohmann::json& j,
      const std::string& key,
      const ValueType& default_value,
      const std::pair<ValueType, bool>& min_bound = { std::numeric_limits<ValueType>::lowest(), true },
      const std::pair<ValueType, bool>& max_bound = { std::numeric_limits<ValueType>::max(), true })
    {
      if (!j.contains(key)) {
        return default_value;
      }
      return get_required_param<ValueType>(j, key, min_bound, max_bound);
    }

    class maker_base {
    public:
      virtual ~maker_base() = default;
      virtual GDALDataset* make(const nlohmann::json& params) = 0;
    };

    template <typename DistributionType, typename RasterValueType>
    class maker;

    template <typename DerivedType>
    class typed_maker_base;

    // The CRTP base class for makers
    template <typename DistributionType, typename RasterValueType>
    class typed_maker_base <maker<DistributionType, RasterValueType> > : public maker_base {
    public:
      GDALDataset* make(const nlohmann::json& j) final 
      {
        auto distribution_params = 
          get_required_param_no_bounds<nlohmann::json>(j, "distribution_parameters");

        auto* derived = static_cast<maker<DistributionType, RasterValueType>*>(this);
        DistributionType dist = derived->distribution_from_json(distribution_params);
        int rows = get_required_param<int>(j, "rows", { 1,true }); // Rows must be at least 1
        int cols = get_required_param<int>(j, "cols", { 1,true }); // Cols must be at least 1

        long long default_seed_val = std::chrono::system_clock::now().time_since_epoch().count();
        long long seed = get_optional_param<long long>(j, "seed", default_seed_val); // Read seed as long long

        int block_rows = get_optional_param<int>(j, "block_rows", 256, { 1,true });
        int block_cols = get_optional_param<int>(j, "block_cols", 256, { 1,true });
        GDALDataType gdal_type = gdal::CXXTypeTraits<RasterValueType>::gdal_type;
        using random_block_generator_type = random_block_generator<DistributionType, RasterValueType>;
        std::unique_ptr<block_generator_interface>  generator = std::make_unique<random_block_generator_type>(seed, rows, cols, block_rows, block_cols, dist);
        return random_raster_dataset::create_from_generator(rows, cols, gdal_type, block_rows, block_cols, std::move(generator));
      }
    };

    // --- value_type_selector ---
    template<GDALDataType GdalDataType> struct value_type_selector {
      using gdal_type = typename gdal::GDALDataTypeTraits<GdalDataType>::type;
      using dist_type = typename gdal::GDALDataTypeTraits<GdalDataType>::type; // Default to GDAL type for distributions
    };

    // Specialization for GDT_Byte: distibution values cannot be unsigned char, therefore use short as smallest fitting type
    template<> struct value_type_selector<GDT_Byte> {
      using gdal_type = typename gdal::GDALDataTypeTraits<GDT_Byte>::type; // unsigned char
      using dist_type = short; // Use 'short' as the internal distribution value type
    };

    template<typename DistributionType, typename RasterValueType>
    class maker<std::uniform_int_distribution<DistributionType>, RasterValueType>
      : public typed_maker_base<maker<std::uniform_int_distribution<DistributionType>, RasterValueType>>
    {
    public:
      std::uniform_int_distribution<DistributionType> distribution_from_json(const nlohmann::json& j) const {
        // Parameters 'a' and 'b' must be read as RasterValueType to validate against its limits.
        auto a = get_optional_param<RasterValueType>(j, "a", std::numeric_limits<RasterValueType>::lowest());
        auto b = get_optional_param<RasterValueType>(j, "b", std::numeric_limits<RasterValueType>::max());
        if (a > b) throw std::runtime_error("For uniform_int_distribution, 'a' must not be greater than 'b'.");
        return std::uniform_int_distribution<DistributionType>(static_cast<DistributionType>(a), static_cast<DistributionType>(b));
      }
    };

    template <typename DistributionType, typename RasterValueType>
    class maker<std::binomial_distribution<DistributionType>, RasterValueType>
      : public typed_maker_base<maker<std::binomial_distribution<DistributionType>, RasterValueType>> {
    public:
      std::binomial_distribution<DistributionType> distribution_from_json(const nlohmann::json& j) const {
        // 't' (number of trials) must be read as RasterValueType to validate against its limits.
        auto t = get_required_param<RasterValueType>(j, "t", { 0, true }); // t >= 0
        auto p = get_required_param<double>(j, "p", { 0.0, true }, { 1.0, true }); // p in [0,1]
        return std::binomial_distribution<DistributionType>(static_cast<DistributionType>(t), p);
      }
    };

    template <typename RasterValueType>
    class maker<std::bernoulli_distribution, RasterValueType>
      : public typed_maker_base<maker<std::bernoulli_distribution, RasterValueType>> {
    public:
      std::bernoulli_distribution distribution_from_json(const nlohmann::json& j) const {
        auto p = get_optional_param<double>(j, "p", 0.5, { 0.0, true }, { 1.0, true }); // p in [0,1]
        return std::bernoulli_distribution(p);
      }
    };

    template <typename DistributionType, typename RasterValueType>
    class maker<std::negative_binomial_distribution<DistributionType>, RasterValueType>
      : public typed_maker_base<maker<std::negative_binomial_distribution<DistributionType>, RasterValueType>> {
    public:
      std::negative_binomial_distribution<DistributionType> distribution_from_json(const nlohmann::json& j) const {
        // 'k' (number of successes) must be read as RasterValueType to validate against its limits.
        auto k = get_required_param<RasterValueType>(j, "k", { 0, false }); // k > 0
        auto p = get_required_param<double>(j, "p", { 0.0, true }, { 1.0, true }); // p in [0,1]
        return std::negative_binomial_distribution<DistributionType>(static_cast<DistributionType>(k), p);
      }
    };

    template <typename DistributionType, typename RasterValueType>
    class maker<std::geometric_distribution<DistributionType>, RasterValueType>
      : public typed_maker_base<maker<std::geometric_distribution<DistributionType>, RasterValueType>> {
    public:
      std::geometric_distribution<DistributionType> distribution_from_json(const nlohmann::json& j) const {
        auto p = get_required_param<double>(j, "p", { 0.0, false }, { 1.0, true }); // p in (0,1]
        return std::geometric_distribution<DistributionType>(p);
      }
    };

    template <typename DistributionType, typename RasterValueType>
    class maker<std::poisson_distribution<DistributionType>, RasterValueType>
      : public typed_maker_base<maker<std::poisson_distribution<DistributionType>, RasterValueType>> {
    public:
      std::poisson_distribution<DistributionType> distribution_from_json(const nlohmann::json& j) const {
        auto mean = get_required_param<double>(j, "mean", { static_cast<double>(0.0), false }); // mean > 0
        return std::poisson_distribution<DistributionType>(mean);
      }
    };

    template <typename ValueType>
    class maker<std::uniform_real_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::uniform_real_distribution<ValueType>, ValueType>> {
    public:
      std::uniform_real_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto a = get_optional_param<ValueType>(j, "a", 0.0);
        auto b = get_optional_param<ValueType>(j, "b", 1.0);
        if (a > b) throw std::runtime_error("For uniform_real_distribution, 'a' must not be greater than 'b'.");
        return std::uniform_real_distribution<ValueType>(a, b);
      }
    };

    template <typename ValueType>
    class maker<std::normal_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::normal_distribution<ValueType>, ValueType>> {
    public:
      std::normal_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto mean = get_optional_param<ValueType>(j, "mean", 0.0);
        auto stddev = get_optional_param<ValueType>(j, "stddev", 1.0, { static_cast<ValueType>(0.0), false }); // stddev > 0
        return std::normal_distribution<ValueType>(mean, stddev);
      }
    };

    template <typename ValueType>
    class maker<std::lognormal_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::lognormal_distribution<ValueType>, ValueType>> {
    public:
      std::lognormal_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto m = get_optional_param<ValueType>(j, "m", 0.0);
        auto s = get_optional_param<ValueType>(j, "s", 1.0, { static_cast<ValueType>(0.0), false }); // s > 0
        return std::lognormal_distribution<ValueType>(m, s);
      }
    };

    template <typename ValueType>
    class maker<std::gamma_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::gamma_distribution<ValueType>, ValueType>> {
    public:
      std::gamma_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto alpha = get_required_param<ValueType>(j, "alpha", { static_cast<ValueType>(0.0), false }); // alpha > 0
        auto beta = get_optional_param<ValueType>(j, "beta", 1.0, { static_cast<ValueType>(0.0), false }); // beta > 0
        return std::gamma_distribution<ValueType>(alpha, beta);
      }
    };

    template <typename ValueType>
    class maker<std::exponential_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::exponential_distribution<ValueType>, ValueType>> {
    public:
      std::exponential_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto lambda = get_optional_param<ValueType>(j, "lambda", 1.0, { static_cast<ValueType>(0.0), false }); // lambda > 0
        return std::exponential_distribution<ValueType>(lambda);
      }
    };

    template <typename ValueType>
    class maker<std::weibull_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::weibull_distribution<ValueType>, ValueType>> {
    public:
      std::weibull_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto a = get_required_param<ValueType>(j, "a", { static_cast<ValueType>(0.0), false }); // a > 0
        auto b = get_required_param<ValueType>(j, "b", { static_cast<ValueType>(0.0), false }); // b > 0
        return std::weibull_distribution<ValueType>(a, b);
      }
    };

    template <typename ValueType>
    class maker<std::extreme_value_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::extreme_value_distribution<ValueType>, ValueType>> {
    public:
      std::extreme_value_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto a = get_optional_param<ValueType>(j, "a", 0.0);
        auto b = get_optional_param<ValueType>(j, "b", 1.0, { static_cast<ValueType>(0.0), false }); // b > 0
        return std::extreme_value_distribution<ValueType>(a, b);
      }
    };

    template <typename ValueType>
    class maker<std::cauchy_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::cauchy_distribution<ValueType>, ValueType>> {
    public:
      std::cauchy_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto a = get_optional_param<ValueType>(j, "a", 0.0);
        auto b = get_optional_param<ValueType>(j, "b", 1.0, {static_cast<ValueType>(0.0), false }); // b > 0
        return std::cauchy_distribution<ValueType>(a, b);
      }
    };

    template <typename ValueType>
    class maker<std::fisher_f_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::fisher_f_distribution<ValueType>, ValueType>> {
    public:
      std::fisher_f_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto m = get_required_param<ValueType>(j, "m", { static_cast<ValueType>(0.0), false }); // m > 0
        auto n = get_required_param<ValueType>(j, "n", { static_cast<ValueType>(0.0), false }); // n > 0
        return std::fisher_f_distribution<ValueType>(m, n);
      }
    };

    template <typename ValueType>
    class maker<std::student_t_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::student_t_distribution<ValueType>, ValueType>> {
    public:
      std::student_t_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto n = get_required_param<ValueType>(j, "n", { static_cast<ValueType>(0.0), false }); // n > 0
        return std::student_t_distribution<ValueType>(n);
      }
    };

    template <typename ValueType>
    class maker<std::chi_squared_distribution<ValueType>, ValueType>
      : public typed_maker_base<maker<std::chi_squared_distribution<ValueType>, ValueType>> {
    public:
      std::chi_squared_distribution<ValueType> distribution_from_json(const nlohmann::json& j) const {
        auto n = get_required_param<ValueType>(j, "n", { static_cast<ValueType>(0.0), false }); // n > 0
        return std::chi_squared_distribution<ValueType>(n);
      }
    };

    // --- Sampling Distributions ---
    template <typename DistributionType, typename RasterValueType>
    class maker<std::discrete_distribution<DistributionType>, RasterValueType>
      : public typed_maker_base<maker<std::discrete_distribution<DistributionType>, RasterValueType>> {
    public:
      std::discrete_distribution<DistributionType> distribution_from_json(const nlohmann::json& j) const {
        auto weights = get_required_param_vector<std::vector<double>>(j, "weights");
        for (const auto& weight : weights) {
          if (weight < 0.0) {
            throw std::runtime_error("For discrete_distribution, all weights must be non-negative.");
          }
        }
        return std::discrete_distribution<DistributionType>(weights.begin(), weights.end());
      }
    };

    template <typename DistributionType, typename RasterValueType>
    class maker<std::piecewise_constant_distribution<DistributionType>, RasterValueType>
      : public typed_maker_base<maker<std::piecewise_constant_distribution<DistributionType>, RasterValueType>> {
    public:
      std::piecewise_constant_distribution<DistributionType> distribution_from_json(const nlohmann::json& j) const {
        // Intervals must be read as RasterValueType for validation, then cast for the distribution.
        auto intervals = get_required_param_vector<std::vector<RasterValueType>>(j, "intervals");
        auto densities = get_required_param_vector<std::vector<double>>(j, "densities");
        if (intervals.size() != densities.size() + 1) {
          throw std::runtime_error("For piecewise_constant, 'intervals' size must be 'densities' size + 1.");
        }
        std::vector<DistributionType> converted_intervals;
        converted_intervals.reserve(intervals.size());
        for (const auto& val : intervals) {
          converted_intervals.push_back(static_cast<DistributionType>(val));
        }
        return std::piecewise_constant_distribution<DistributionType>(converted_intervals.begin(), converted_intervals.end(), densities.begin());
      }
    };

    template <typename DistributionType, typename RasterValueType>
    class maker<std::piecewise_linear_distribution<DistributionType>, RasterValueType>
      : public typed_maker_base<maker<std::piecewise_linear_distribution<DistributionType>, RasterValueType>> {
    public:
      std::piecewise_linear_distribution<DistributionType> distribution_from_json(const nlohmann::json& j) const {
        // Intervals must be read as RasterValueType for validation, then cast for the distribution.
        auto intervals = get_required_param_vector<std::vector<RasterValueType>>(j, "intervals");
        auto densities = get_required_param_vector<std::vector<double>>(j, "densities");
        if (intervals.size() != densities.size()) {
          throw std::runtime_error("For piecewise_linear, 'intervals' size must be equal to 'densities' size.");
        }
        std::vector<DistributionType> converted_intervals;
        converted_intervals.reserve(intervals.size());
        for (const auto& val : intervals) {
          converted_intervals.push_back(static_cast<DistributionType>(val));
        }
        return std::piecewise_linear_distribution<DistributionType>(converted_intervals.begin(), converted_intervals.end(), densities.begin());
      }
    };

    template<GDALDataType GdalDataType>
    std::unique_ptr<maker_base> get_maker_int(distribution_type dt) {
      using gdal_type = typename value_type_selector<GdalDataType>::gdal_type;
      using dist_type = typename value_type_selector<GdalDataType>::dist_type;

      switch (dt) {
      case distribution_type::uniform_integer:
        return std::make_unique<maker<std::uniform_int_distribution<dist_type>, gdal_type>>();
      case distribution_type::bernoulli:
        return std::make_unique<maker<std::bernoulli_distribution, gdal_type>>();
      case distribution_type::binomial:
        return std::make_unique<maker<std::binomial_distribution<dist_type>, gdal_type>>();
      case distribution_type::negative_binomial:
        return std::make_unique<maker<std::negative_binomial_distribution<dist_type>, gdal_type>>();
      case distribution_type::geometric:
        return std::make_unique<maker<std::geometric_distribution<dist_type>, gdal_type>>();
      case distribution_type::poisson:
        return std::make_unique<maker<std::poisson_distribution<dist_type>, gdal_type>>();
      case distribution_type::discrete:
        return std::make_unique<maker<std::discrete_distribution<dist_type>, gdal_type>>();
      default:
        throw std::runtime_error("Distribution type '" + to_string(dt) +
          "' is not an integer distribution compatible with GDALDataType " +
          GDALGetDataTypeName(GdalDataType) + ".");
      }
    }

    template<GDALDataType GdalDataType>
    std::unique_ptr<maker_base> get_maker_real(distribution_type dt) {
      using gdal_type = typename value_type_selector<GdalDataType>::gdal_type;
      using dist_type = typename value_type_selector<GdalDataType>::dist_type;
      switch (dt) {
      case distribution_type::uniform_real:
        return std::make_unique<maker<std::uniform_real_distribution<dist_type>, gdal_type>>();
      case distribution_type::normal:
        return std::make_unique<maker<std::normal_distribution<dist_type>, gdal_type>>();
      case distribution_type::lognormal:
        return std::make_unique<maker<std::lognormal_distribution<dist_type>, gdal_type>>();
      case distribution_type::gamma:
        return std::make_unique<maker<std::gamma_distribution<dist_type>, gdal_type>>();
      case distribution_type::exponential:
        return std::make_unique<maker<std::exponential_distribution<dist_type>, gdal_type>>();
      case distribution_type::weibull:
        return std::make_unique<maker<std::weibull_distribution<dist_type>, gdal_type>>();
      case distribution_type::extreme_value:
        return std::make_unique<maker<std::extreme_value_distribution<dist_type>, gdal_type>>();
      case distribution_type::cauchy:
        return std::make_unique < maker<std::cauchy_distribution<dist_type>, gdal_type>>();
      case distribution_type::fisher_f:
        return std::make_unique<maker<std::fisher_f_distribution<dist_type>, gdal_type>>();
      case distribution_type::student_t:
        return std::make_unique<maker<std::student_t_distribution<dist_type>, gdal_type>>();
      case distribution_type::chi_squared:
        return std::make_unique<maker<std::chi_squared_distribution<dist_type>, gdal_type>>();
      case distribution_type::piecewise_constant:
        return std::make_unique<maker<std::piecewise_constant_distribution<dist_type>, gdal_type>>();
      case distribution_type::piecewise_linear:
        return std::make_unique<maker<std::piecewise_linear_distribution<dist_type>, gdal_type>>();

      default:
        throw std::runtime_error("Distribution type '" + to_string(dt) +
          "' is not a real or sampling distribution compatible with GDALDataType " +
          GDALGetDataTypeName(GdalDataType) + ".");
      }
    }

    std::unique_ptr<maker_base> get_maker(distribution_type dt, GDALDataType gdt) {
      switch (gdt) {
      case GDT_Byte:    return get_maker_int<GDT_Byte>(dt);
      case GDT_UInt16:  return get_maker_int<GDT_UInt16>(dt);
      case GDT_Int16:    return get_maker_int<GDT_Int16>(dt);
      case GDT_UInt32:  return get_maker_int<GDT_UInt32>(dt);
      case GDT_Int32:    return get_maker_int<GDT_Int32>(dt);
      case GDT_UInt64:  return get_maker_int<GDT_UInt64>(dt);
      case GDT_Int64:    return get_maker_int<GDT_Int64>(dt);
      case GDT_Float32: return get_maker_real<GDT_Float32>(dt);
      case GDT_Float64: return get_maker_real<GDT_Float64>(dt);
      default:
        throw std::runtime_error("Unsupported GDALDataType '" + std::string(GDALGetDataTypeName(gdt)) + "' for random generation.");
      }
    }

    GDALDataset* random_raster_dataset::create_from_json(const nlohmann::json& j) {
      auto data_type_str = get_required_param_no_bounds<std::string>(j, "data_type");
      GDALDataType gdt = GDALGetDataTypeByName(data_type_str.c_str());
      if (gdt == GDT_Unknown) {
        throw std::runtime_error("Unknown or unsupported GDAL data type: " + data_type_str);
      }

      auto dist_type_str = get_required_param_no_bounds<std::string>(j, "distribution");
      distribution_type dt = string_to_distribution_type(dist_type_str);

      std::unique_ptr<maker_base> maker_ptr = get_maker(dt, gdt);

      return maker_ptr->make(j);
    }
  }
}
