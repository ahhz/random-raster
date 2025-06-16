#pragma once

#include <pronto/raster/block_generator_interface.h>
#include <pronto/raster/random_dataset.h>
#include <random>

namespace pronto {
namespace raster {


// Fills a block with random numbers based on a given distribution and generator,
// also calculates statistics.
template<class Distribution, class Generator = std::mt19937_64>
class random_block_generator : public block_generator_interface
{
public:
  using value_type = typename Distribution::result_type;

  // Constructor: Takes a base seed, total columns (for block seed calculation),
  // block column size, and the distribution object.
  random_block_generator(uint64_t base_seed, int total_cols, int block_col_size, Distribution distribution)
      : m_base_seed(base_seed),
        m_blocks_in_row(1 + (total_cols - 1) / block_col_size),
        m_distribution(distribution)
  {
  }

  // Fills a block of data with random values.
  void fill_block_void(int major_row, int major_col, void* p_data, size_t block_size) override {
    value_type* block_begin = static_cast<value_type*>(p_data);

    // Derives a unique seed for this block for reproducible results.
    uint64_t block_seed = m_base_seed + (uint64_t)major_row * m_blocks_in_row + major_col;
    Generator rng(block_seed);

    // Fill the block with random numbers.
    std::generate(block_begin, block_begin + block_size,
                  [&]() { return m_distribution(rng); });
  }

  // --- Statistical methods ---
  double get_min() const override {
    return static_cast<double>(m_distribution.min());
  }

  double get_max() const override {
    return static_cast<double>(m_distribution.max());
  }

  double get_mean() const override {
    return (get_min() + get_max()) / 2.0;
  }

  double get_std_dev() const override {
    return (get_max() - get_min()) / std::sqrt(12.0);
  }

private:
  uint64_t     m_base_seed;
  int          m_blocks_in_row;
  Distribution m_distribution;
};

} // namespace raster
} // namespace pronto