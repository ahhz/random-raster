//=======================================================================
// Copyright 2024-2025
// Author: Alex Hagen-Zanker
// University of Surrey
//
// Distributed under the MIT Licence (http://opensource.org/licenses/MIT)
//=======================================================================
//
// Abstract base class for filling raster blocks with generated values 
// and providing statistics.
   
#pragma once

namespace pronto {
  namespace raster {
    class block_generator_interface {
    public:
      virtual ~block_generator_interface() = default;
      virtual void fill_block(int major_row, int major_col, void* block, size_t block_size) = 0;
      virtual double get_min() const = 0;
      virtual double get_max() const = 0;
      virtual double get_mean() const = 0;
      virtual double get_std_dev() const = 0;
    };
  } // namespace raster
} // namespace pronto