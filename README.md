# Pronto Raster Random Raster
This library is a stand-alone component of Pronto Raster. It is a virtual raster format that used standard libary random distributions to generate GDALRasterbands. It combines the std::random functionality to obtain random values conform specified distributions. The random values are loaded on demand block-by-block and the blocks are managed by GDAL and its caching system.

The library requires the following:
- C++14
- GDAL 
- rlohmann/JSON

See the documentation [here](https://github.com/ahhz/random-raster/blob/main/docs/random_raster_driver.md)
   
