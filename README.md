# Pronto Raster Random Raster
 <table style="width:100%">
  <tr>
    <td><img src="./docs/assets/logo.svg" width="350"> </td>
    <td>The Pronto Raster Library is a C++ library to work with raster data. The core idea of the library is to make raster data accessible following the Range concept, making it possible to work with them using standard and modern C++ idioms. The library makes it straightforward to implement Map Algebra operations, including Moving Window analysis.</td>
    <td>This library is a stand-alone component of Pronto Raster. It is a virtual raster format that used standard libary random distributions to generate GDALRasterbands. It combines the std::random functionality to obtain random values conform specified distributions. The random values are loaded on demand block-by-block and the blocks are managed by GDAL and its caching system</td>
  </tr>
</table> 

The library requires the following:
- C++20 
- GDAL 
- rlohmann/JSON

   
