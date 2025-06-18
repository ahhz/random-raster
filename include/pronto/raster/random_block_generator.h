//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================
//
// Class for filling GDAL raster blocks with random values from std 
// library distributions

#pragma once

#include <pronto/raster/block_generator_interface.h>

#include <algorithm> // For std::min and std::max
#include <cmath> // For std::sqrt
#include <numeric> // For std::numeric_limits
#include <random>
#include <type_traits> // For std::is_same

namespace pronto {
  namespace raster {

    template<class Distribution, typename TargetGdalType, class Generator = std::mt19937_64>
    class random_block_generator : public block_generator_interface
    {
    public:
      using distribution_result_type = typename Distribution::result_type;
        // Typically the distribution_result_type is the same as TargetGdalType, 
        // however in some cases the distribution does not support the exact type. 
        // In particular GDAL has GDT_Byte which is an unsigned char and this is 
        // not an integer type supported by the standard library distributions. 
        // They will produce shorts which we then cast to unsigned char.
        //  
        // Note GDAL uses x_size and y_size, pronto raster uses rows (for x_size) 
        // and cols (for y_size)


      random_block_generator(uint64_t base_seed, int rows, int cols,
        int block_rows, int block_cols, Distribution distribution)
        : m_base_seed(base_seed),
        m_rows(rows),
        m_cols(cols),
        m_block_rows(block_rows),
        m_block_cols(block_cols),
        m_blocks_in_row(1 + (cols - 1) / block_cols), // Calculate blocks per row
        m_distribution(distribution)
      {
      }

      // Fills a block of data with random values.
      void fill_block(int major_row, int major_col, void* block, size_t num_elements_in_block) override {
        
        // Cast the void pointer to the target GDAL data type pointer.
        TargetGdalType* block_begin = static_cast<TargetGdalType*>(block);

        // Derives a unique seed for this block for reproducible results.
        uint64_t block_seed = m_base_seed +
          static_cast<uint64_t>(major_row) * m_blocks_in_row +
          static_cast<uint64_t>(major_col);
        
        // Fill the block with random values using the distribution.
        Generator rng(block_seed);
        std::generate(block_begin, block_begin + num_elements_in_block,
          [&]() {
            return static_cast<TargetGdalType>(m_distribution(rng));
          });
      }

      // --- Statistical properties ---
      // These methods provide the theoretical min/max/mean/std_dev of the distribution.
      // They are accurate for bounded distributions (like uniform, bernoulli)
      // but might not be meaningful or precise for unbounded ones (like normal, poisson with large mean).
      double get_min() const override {
        try {
          return static_cast<double>(m_distribution.min());
        }
        catch (const std::exception& e) {
          return std::numeric_limits<double>::lowest();
        }
      }

      double get_max() const override {
        try {
          return static_cast<double>(m_distribution.max());
        }
        catch (const std::exception& e) {
          return std::numeric_limits<double>::max();
        }
      }

      double get_mean() const override {
        double min_val = get_min();
        double max_val = get_max();
        if (min_val != std::numeric_limits<double>::lowest() && max_val != std::numeric_limits<double>::max()) {
          return (min_val + max_val) / 2.0;
        }
        return 0.0;
      }

      double get_std_dev() const override {
        double range = get_max() - get_min();
        if (range >= 0) {
          return range / std::sqrt(12.0);
        }
        return 0.0;
      }

    private:
      uint64_t     m_base_seed;
      int      m_rows;
      int      m_cols;
      int      m_block_rows;
      int      m_block_cols;
      int      m_blocks_in_row;
      Distribution   m_distribution;
    };

  } // namespace raster
} // namespace pronto