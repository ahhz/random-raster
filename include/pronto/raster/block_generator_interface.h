#pragma once

namespace pronto {
namespace raster {

// --- Interface for any block generator ---
// Abstract base class for filling raster blocks with generated values and providing statistics.
class block_generator_interface {
public:
  virtual ~block_generator_interface() = default;

  // Fills the block with generated values.
  virtual void fill_block_void(int major_row, int major_col, void* p_data, size_t block_size) = 0;

  // Returns the minimum possible value generated.
  virtual double get_min() const = 0;
  // Returns the maximum possible value generated.
  virtual double get_max() const = 0;
  // Returns the mean of the distribution.
  virtual double get_mean() const = 0;
  // Returns the standard deviation of the distribution.
  virtual double get_std_dev() const = 0;
};
} // namespace raster
} // namespace pronto