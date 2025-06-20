# GDAL Random Raster Format (custom format)

This document provides usage instructions for the custom GDAL raster format that generates rasters of random values. This format leverages the GDAL caching mechanism for efficient data access and the distributions provided in the <random> library of the C++ standard library (since C++11).

## Introduction

The custom GDAL random raster format allows you to create virtual raster datasets filled with random values drawn from various standard statistical distributions. The driver supports regular and virtual files in JSON format, as well as anxillary files with spatial reference and geotransformation information.. 

## Usage

A Random Raster is opened by opening a JSON file (extension .json) that contains the settings for producing the random raster. The file can be opened using ```GDALOpen```.

Here's the general structure of the JSON string:
```json
{
  "type": "RANDOM_RASTER",
  "rows": <integer>,
  "cols": <integer>,
  "data_type": "<GDALDataType string>",
  "seed": <unsigned integer>,
  "block_rows": <integer>,
  "block_cols": <integer>,
  "distribution": "<distribution_type string>",
  "distribution_parameters": {
    // Parameters specific to the chosen distribution
  }
}
```
## Top-Level Parameters

* ```type```: (Required, string) Must be set to ```"RANDOM_RASTER"```. This identifies the custom driver.
* ```rows```: (Required, integer) The number of rows (height) of the generated raster.
* ```cols```: (Required, integer) The number of columns (width) of the generated raster.
* ```data_type```: (Required, string) The data type for the raster bands. Supported values are:
    * ```"Byte"``` (Unsigned 8-bit integer)
    * ```"UInt16"``` (Unsigned 16-bit integer)
    * ```"Int16"``` (Signed 16-bit integer)
    * ```"UInt32"``` (Unsigned 32-bit integer)
    * ```"Int32"``` (Signed 32-bit integer)
    * ```"UInt64"``` (Unsigned 64-bit integer)
    * ```"Int64"``` (Signed 64-bit integer)
    * ```"Float32"``` (32-bit floating point)
    * ```"Float64"``` (64-bit floating point)
* ```seed```: (Optional, unsigned integer) The seed for the random number generator. If not provided, a time-based seed is used, making each raster unique. Providing a seed ensures reproducibility.
* ```block_rows```: (Optional, integer) The height of internal blocks used by GDAL for caching. Defaults to 256 if not specified.
* ```block_cols```: (Optional, integer) The width of internal blocks used by GDAL for caching. Defaults to 256 if not specified.
* ```distribution```: (Required, string) The type of statistical distribution to use for generating random values. See "Supported Distributions and Parameters" for available options.
* ```distribution_parameters```: (Required, JSON object) A JSON object containing the specific parameters for the chosen distribution. The required parameters vary depending on the distribution type.

---

## Supported Distributions and Parameters

This custom format supports a wide range of standard C++ random distributions. The choice of distribution depends on whether you're generating integer or floating-point raster data.

*Note on ```Byte```: While ```Byte``` represents unsigned 8-bit integers (0-255), some underlying C++ standard library distributions don't directly support ```unsigned char```. Internally, ```short``` is used for the distribution, and the results are then cast to ``unsigned char```. *

### Integer Distributions

These distributions are suitable for ```Byte```, ```UInt16```, ```Int16```, ```UInt32```, ```Int32```, ```UInt64```, and ```Int64``` data types.

1.  ```uniform_integer```
    * Description: Generates uniformly distributed random integers in a specified range.
    * Parameters:
        * ```a```: (Integer) The inclusive lower bound of the range.
            * Default: std::numeric_limits<Type>::min() (e.g., 0 for ```Byte```, -32768 for ```Int16```).
        * ```b```: (Integer) The inclusive upper bound of the range.
            * Default: std::numeric_limits<Type>::max() (e.g., 255 for ```Byte```, 32767 for ```Int16```).
    * Constraints: ```a``` <= ```b```. For ```Byte```, ```a``` and ```b``` must be between 0 and 255.

2.  ```bernoulli```
    * Description: Generates 0 or 1 with a specified probability.
    * Parameters:
        * ```p```: (Double) The probability of success (generating 1).
            * Default: 0.5.
    * Constraints: 0.0 <= ```p``` <= 1.0.

3.  ```binomial```
    * Description: Generates the number of successes in a sequence of independent Bernoulli trials.
    * Parameters:
        * ```t```: (Integer) The number of trials.
            * Default: 1.
        * ```p```: (Double) The probability of success on each trial.
            * Default: 0.5.
    * Constraints: ```t``` >= 0. 0.0 <= ```p``` <= 1.0. For ```Byte```, ```t``` should ideally not exceed the theoretical maximum of ```std::numeric_limits<Type>::max()```.

4.  ```negative_binomial```
    * Description: Generates the number of failures before a specified number of successes.
    * Parameters:
        * ```k```: (Integer) The number of successes.
            * Default: 1.
        * ```p```: (Double) The probability of success on each trial.
            * Default: 0.5.
    * Constraints: ```k``` > 0. 0.0 < ```p``` <= 1.0.

5.  ```geometric```
    * Description: Generates the number of trials until the first success.
    * Parameters:
        * ```p```: (Double) The probability of success on each trial.
            * Default: 0.5.
    * Constraints: 0.0 < ```p``` <= 1.0.

6.  ```poisson```
    * Description: Generates the number of events occurring in a fixed interval of time or space.
    * Parameters:
        * ```mean```: (Double) The mean (lambda) of the distribution.
            * Default: 1.0.
    * Constraints: ```mean``` > 0.0.

7.  ```discrete_distribution```
    * Description: Generates random integers based on a set of user-defined weights.
    * Parameters:
        * ```weights```: (Array of Doubles) A non-empty list of non-negative weights. The probability of generating index ```i``` is proportional to ```weights[i]```.
    * Constraints: ```weights``` array must contain at least one element.

### Real Distributions

These distributions are suitable for ```Float32``` and ```Float64``` data types.

1.  ```uniform_real```
    * Description: Generates uniformly distributed random real numbers in a specified range.
    * Parameters:
        * ```a```: (Float/Double) The inclusive lower bound of the range.
            * Default: ```std::numeric_limits<Type>::lowest()```.
        * ```b```: (Float/Double) The inclusive upper bound of the range.
            * Default: ```std::numeric_limits<Type>::max()```.
    * Constraints: ```a``` <= ```b```.

2.  ```weibull```
    * Description: Generates random numbers according to the Weibull distribution.
    * Parameters:
        * ```a```: (Float/Double) The shape parameter (k or alpha).
            * Default: 1.0.
        * ```b```: (Float/Double) The scale parameter (lambda or beta).
            * Default: 1.0.
    * Constraints: ```a``` > 0.0, ```b``` > 0.0.

3.  ```extreme_value```
    * Description: Generates random numbers according to the Extreme Value (Gumbel) distribution.
    * Parameters:
        * ```a```: (Float/Double) The location parameter (mu).
            * Default: 0.0.
        * ```b```: (Float/Double) The scale parameter (sigma).
            * Default: 1.0.
    * Constraints: ```b``` > 0.0.

4.  ```cauchy```
    * Description: Generates random numbers according to the Cauchy distribution.
    * Parameters:
        * ```a```: (Float/Double) The location parameter (x0).
            * Default: 0.0.
        * ```b```: (Float/Double) The scale parameter (gamma).
            * Default: 1.0.
    * Constraints: ```b``` > 0.0.

5.  ```normal```
    * Description: Generates random numbers according to the Normal (Gaussian) distribution.
    * Parameters:
        * ```mean```: (Float/Double) The mean (mu) of the distribution.
            * Default: 0.0.
        * ```stddev```: (Float/Double) The standard deviation (sigma) of the distribution.
            * Default: 1.0.
    * Constraints: ```stddev``` >= 0.0.

6.  ```exponential```
    * Description: Generates random numbers according to the Exponential distribution.
    * Parameters:
        * ```lambda```: (Float/Double) The rate parameter (lambda).
            * Default: 1.0.
    * Constraints: ```lambda``` > 0.0.

7.  ```gamma```
    * Description: Generates random numbers according to the Gamma distribution.
    * Parameters:
        * ```alpha```: (Float/Double) The shape parameter (alpha or k).
            * Default: 1.0.
        * ```beta```: (Float/Double) The scale parameter (beta or theta).
            * Default: 1.0.
    * Constraints: ```alpha``` > 0.0, ```beta``` > 0.0.

8.  ```lognormal```
    * Description: Generates random numbers according to the Log-normal distribution.
    * Parameters:
        * ```m```: (Float/Double) The mean (mu) of the underlying normal distribution.
            * Default: 0.0.
        * ```s```: (Float/Double) The standard deviation (sigma) of the underlying normal distribution.
            * Default: 1.0.
    * Constraints: ```s``` >= 0.0.

9.  ```fisher_f```
    * Description: Generates random numbers according to the Fisher-F distribution.
    * Parameters:
        * ```m```: (Float/Double) Degrees of freedom for the numerator.
            * Default: 1.0.
        * ```n```: (Float/Double) Degrees of freedom for the denominator.
            * Default: 1.0.
    * Constraints: ```m``` > 0.0, ```n``` > 0.0.

10. ```student_t```
    * Description: Generates random numbers according to the Student's t-distribution.
    * Parameters:
        * ```n```: (Float/Double) Degrees of freedom.
            * Default: 1.0.
    * Constraints: ```n``` > 0.0.

11. ```piecewise_constant```
    * Description: Generates random numbers from a piecewise constant probability density function.
    * Parameters:
        * ```intervals```: (Array of Floats/Doubles) A sorted list of interval boundaries.
        * ```densities```: (Array of Floats/Doubles) A list of density values for each interval.
    * Constraints: ```intervals``` must have at least two elements. The number of ```densities``` must be ```intervals.size() - 1```.

12. ```piecewise_linear```
    * Description: Generates random numbers from a piecewise linear probability density function.
    * Parameters:
        * ```intervals```: (Array of Floats/Doubles) A sorted list of interval boundaries.
        * ```densities```: (Array of Floats/Doubles) A list of density values at each interval boundary.
    * Constraints: ```intervals``` must have at least two elements. The number of ```densities``` must be ```intervals.size()```.

---

## Example Usage (C++)

The following C++ example demonstrates how to open a random raster dataset using the custom GDAL format and read some pixel values. This example generates a 256x512 raster of Byte values, with values uniformly distributed between 1 and 6 (inclusive), mimicking a dice roll.

```cpp
#include <iostream> 
#include <string> 
#include <algorithm> 
#include <vector>    

#include <gdal.h>      
#include <gdal_priv.h> 
#include <cpl_vsi.h>   
#include <cpl_error.h> 
#include <cpl_conv.h>  

#ifndef RANDOM_RASTER_DRIVER_PATH
#define RANDOM_RASTER_DRIVER_PATH "path/to/the/gdal_RANDOM_RASTER_shared_library/directory" // Consider making this an env var or cmd line arg
#endif

// Function to check if the RANDOM_RASTER driver is installed
bool check_driver_installed() {
  GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("RANDOM_RASTER");
  return (driver != nullptr);
}

// Function to manually install the RANDOM_RASTER driver
bool install_driver_manually() {
  const char* restore_path = CPLGetConfigOption("GDAL_DRIVER_PATH", nullptr);
  CPLSetConfigOption("GDAL_DRIVER_PATH", RANDOM_RASTER_DRIVER_PATH);
  GetGDALDriverManager()->AutoLoadDrivers(); // This will load drivers from RANDOM_RASTER_DRIVER_PATH
  CPLSetConfigOption("GDAL_DRIVER_PATH", restore_path); // Restore original path
  return check_driver_installed();
}

int main() {
  GDALAllRegister(); // Register all GDAL drivers

  if (check_driver_installed()) {
    std::cout << "RANDOM_RASTER driver automatically registered successfully!" << std::endl;
  }
  else if (install_driver_manually()) {
    std::cout << "RANDOM_RASTER driver manually registered successfully!" << std::endl;
  }
  else {
    std::cerr << "RANDOM_RASTER driver not found. Please ensure RANDOM_RASTER_DRIVER_PATH is correct and the driver is built." << std::endl;
    std::cerr << "Current RANDOM_RASTER_DRIVER_PATH set to: " << RANDOM_RASTER_DRIVER_PATH << std::endl;
    GDALDestroyDriverManager(); 
    return 1; 
  }

  const std::string json_params =
    "{\n"
    "  \"type\": \"RANDOM_RASTER\",\n"
    "  \"rows\": 256,\n"
    "  \"cols\": 512,\n"
    "  \"data_type\": \"Byte\",\n"
    "  \"seed\": 1234,\n"
    "  \"block_rows\": 64,\n"
    "  \"block_cols\": 64,\n"
    "  \"distribution\": \"uniform_integer\",\n"
    "  \"distribution_parameters\": {\n"
    "    \"a\": 1,\n"
    "    \"b\": 6\n"
    "  }\n"
    "}";

  // --- Start of VSI Memory File Integration ---
  const char* vsi_mem_filename = "/vsimem/random_raster_params.json";

  VSILFILE* fp_vsimem = VSIFileFromMemBuffer(
    vsi_mem_filename,
    reinterpret_cast<GByte*>(const_cast<char*>(json_params.c_str())),
    json_params.length(),
    FALSE
  );

  if (fp_vsimem == nullptr) {
    std::cerr << "Error: Could not create VSI memory file for JSON parameters." << std::endl;
    std::cerr << "GDAL Last Error: " << CPLGetLastErrorMsg() << std::endl;
    GDALDestroyDriverManager();
    return 1;
  }
  VSIFCloseL(fp_vsimem);

  GDALDataset* dataset = static_cast<GDALDataset*>(GDALOpen(vsi_mem_filename, GA_ReadOnly));

  if (dataset == nullptr) {
    std::cerr << "Error: Could not open dataset from VSI memory file. " << CPLGetLastErrorMsg() << std::endl;
    VSIUnlink(vsi_mem_filename); 
    GDALDestroyDriverManager();
    return 1;
  }
  // --- End of VSI Memory File Integration ---

  GDALRasterBand* band = dataset->GetRasterBand(1);
  if (band == nullptr) {
    std::cerr << "Error: Could not get raster band." << std::endl;
    GDALClose(dataset);
    VSIUnlink(vsi_mem_filename); 
    GDALDestroyDriverManager();
    return 1;
  }

  int width = band->GetXSize();
  int height = band->GetYSize();
  std::cout << "Raster width: " << width << ", height: " << height << std::endl;

  int block_x_size, block_y_size;
  band->GetBlockSize(&block_x_size, &block_y_size);
  std::cout << "Block width: " << block_x_size << ", height: " << block_y_size << std::endl;

  const int size_of_element = 1;
  std::vector<GByte> block_data(static_cast<size_t>(block_x_size) * block_y_size * size_of_element);

  CPLErr err = band->ReadBlock(0, 0, block_data.data());
  if (err != CE_None) {
    std::cerr << "Error: Could not read block: " << CPLGetLastErrorMsg() << std::endl;
    GDALClose(dataset);
    VSIUnlink(vsi_mem_filename);
    GDALDestroyDriverManager();
    return 1;
  }

  std::cout << "First few values from the block (should be between 1 and 6):" << std::endl;
  for (int i = 0; i < std::min(10, block_x_size * block_y_size); ++i) {
    std::cout << static_cast<int>(block_data[i]) << " ";
  }
  std::cout << std::endl;

  GDALClose(dataset);
  VSIUnlink(vsi_mem_filename); 
  GDALDestroyDriverManager();

  return 0;
}