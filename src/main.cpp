#ifndef GDAL_BUILD_AS_PLUGIN
int main()
{
  GDALAllRegister();
  pronto::raster::register_random_driver();

  std::cout << "--- Testing direct C++ instantiation with parameters object ---" << std::endl;
  pronto::raster::random_raster_parameters params1;
  params1.m_width = 500;
  params1.m_height = 300;
  params1.m_data_type = GDT_Float32;
  params1.m_bits = 32;
  params1.m_seed = static_cast<unsigned int>(std::time(nullptr));
  params1.m_block_x_size = 128;
  params1.m_block_y_size = 128;
  params1.m_type = "RANDOM_RASTER";
  // Set a specific geotransform and projection for testing
  params1.m_geo_transform = {100.0, 0.5, 0.0, 200.0, 0.0, -0.5}; // Example: origin (100,200), 0.5 unit/pixel
  params1.m_projection_wkt = "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AXIS[\"Latitude\",NORTH],AXIS[\"Longitude\",EAST],AUTHORITY[\"EPSG\",\"4326\"]]";

  if (!params1.validate()) {
    std::cerr << "Validation failed for params1. Exiting." << std::endl;
    return 1;
  }

  GDALDataset* ds1 = pronto::raster::random_dataset::create(params1);

  if (ds1 == nullptr) {
    std::cerr << "Failed to create random_dataset via create(params) method." << std::endl;
    return 1;
  }

  std::cout << "Dataset 1 opened successfully." << std::endl;
  std::cout << "Raster size: " << ds1->GetRasterXSize() << "x" << ds1->GetRasterYSize() << std::endl;
  std::cout << "Band count: " << ds1->GetRasterCount() << std::endl;
  std::cout << "Parameters JSON string: " << params1.to_json().dump(2) << std::endl;

  double gt1[6];
  ds1->GetGeoTransform(gt1);
  std::cout << "Retrieved GeoTransform: [";
  for (int i = 0; i < 6; ++i) {
      std::cout << gt1[i] << (i == 5 ? "" : ", ");
  }
  std::cout << "]" << std::endl;
  std::cout << "Retrieved Projection WKT: " << ds1->GetProjectionRef() << std::endl;


  if (ds1->GetRasterCount() > 0) {
    GDALRasterBand* band = ds1->GetRasterBand(1);
    std::cout << "Band 1 data type: " << GDALGetDataTypeName(band->GetRasterDataType()) << std::endl;
    std::cout << "Band 1 block size: " << band->GetBlockXSize() << "x" << band->GetBlockYSize() << std::endl;

    double min_val, max_val, mean_val, std_dev_val;
    band->GetStatistics(TRUE, FALSE, &min_val, &max_val, &mean_val, &std_dev_val);
    std::cout << "Stats: Min=" << min_val << ", Max=" << max_val << ", Mean=" << mean_val << ", StdDev=" << std_dev_val << std::endl;

    float f_pixel_value;
    band->ReadRaster(0, 0, 1, 1, &f_pixel_value, 1, 1, GDT_Float32, 0, 0);
    std::cout << "Pixel (0,0) value: " << f_pixel_value << std::endl;
  }
  GDALClose(ds1);
  std::cout << "Dataset 1 closed." << std::endl;

  std::cout << "\n--- Testing direct C++ instantiation with default block size ---" << std::endl;
  pronto::raster::random_raster_parameters params3;
  params3.m_width = 600;
  params3.m_height = 400;
  params3.m_data_type = GDT_UInt16;
  params3.m_bits = 16;
  params3.m_seed = 54321;
  params3.m_type = "RANDOM_RASTER";
  // Default geotransform and projection will be used here

  if (!params3.validate()) {
    std::cerr << "Validation failed for params3. Exiting." << std::endl;
    return 1;
  }

  GDALDataset* ds3 = pronto::raster::random_dataset::create(params3);

  if (ds3 == nullptr) {
    std::cerr << "Failed to create random_dataset via create(params) for DS3." << std::endl;
    return 1;
  }

  std::cout << "Dataset 3 opened successfully." << std::endl;
  std::cout << "Raster size: " << ds3->GetRasterXSize() << "x" << ds3->GetRasterYSize() << std::endl;
  std::cout << "Band count: " << ds3->GetRasterCount() << std::endl;
  std::cout << "Parameters JSON string: " << params3.to_json().dump(2) << std::endl;

  double gt3[6];
  ds3->GetGeoTransform(gt3);
  std::cout << "Retrieved GeoTransform: [";
  for (int i = 0; i < 6; ++i) {
      std::cout << gt3[i] << (i == 5 ? "" : ", ");
  }
  std::cout << "]" << std::endl;
  std::cout << "Retrieved Projection WKT: " << ds3->GetProjectionRef() << std::endl;


  if (ds3->GetRasterCount() > 0) {
    GDALRasterBand* band = ds3->GetRasterBand(1);
    std::cout << "Band 1 data type: " << GDALGetDataTypeName(band->GetRasterDataType()) << std::endl;
    std::cout << "Band 1 block size: " << band->GetBlockXSize() << "x" << band->GetBlockYSize() << std::endl;
    if (band->GetBlockXSize() != 256 || band->GetBlockYSize() != 256) {
      std::cerr << "ERROR: Default block size not applied correctly for DS3 (expected 256x256)!" << std::endl;
    }

    double min_val, max_val, mean_val, std_dev_val;
    band->GetStatistics(TRUE, FALSE, &min_val, &max_val, &mean_val, &std_dev_val);
    std::cout << "Stats: Min=" << min_val << ", Max=" << max_val << ", Mean=" << mean_val << ", StdDev=" << std_dev_val << std::endl;
  }
  GDALClose(ds3);
  std::cout << "Dataset 3 closed." << std::endl;


  std::cout << "\n--- Testing GDALOpen with direct JSON string ---" << std::endl;
  const char* json_params_direct_str = R"({
    "type": "RANDOM_RASTER",
    "width": 1000,
    "height": 700,
    "data_type": "Byte",
    "bits": 8,
    "seed": 12345,
    "block_x_size": 64,
    "block_y_size": 64,
    "geo_transform": [10.5, 0.1, 0.0, 20.5, 0.0, -0.1],
    "projection_wkt": "PROJCS[\"UTM Zone 10, Northern Hemisphere\",GEOGCS[\"GRS 1980(IUGG, 1980)\",DATUM[\"unknown\",SPHEROID[\"GRS80\",6378137,298.257222101],TOWGS84[0,0,0,0,0,0,0]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",-123],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],UNIT[\"Meter\",1]]"
  })";
  GDALDataset* ds2 = (GDALDataset*)GDALOpen(json_params_direct_str, GA_ReadOnly);

  if (ds2 == nullptr) {
    std::cerr << "Failed to open RANDOM_RASTER dataset via GDALOpen (direct JSON string)." << std::endl;
    return 1;
  }

  std::cout << "Dataset 2 (GDALOpen, direct JSON) opened successfully." << std::endl;
  std::cout << "Raster size: " << ds2->GetRasterXSize() << "x" << ds2->GetRasterYSize() << std::endl;
  std::cout << "Band count: " << ds2->GetRasterCount() << std::endl;

  double gt2[6];
  ds2->GetGeoTransform(gt2);
  std::cout << "Retrieved GeoTransform: [";
  for (int i = 0; i < 6; ++i) {
      std::cout << gt2[i] << (i == 5 ? "" : ", ");
  }
  std::cout << "]" << std::endl;
  std::cout << "Retrieved Projection WKT: " << ds2->GetProjectionRef() << std::endl;


  if (ds2->GetRasterCount() > 0) {
    GDALRasterBand* band = ds2->GetRasterBand(1);
    std::cout << "Band 1 data type: " << GDALGetDataTypeName(band->GetRasterDataType()) << std::endl;
    std::cout << "Band 1 block size: " << band->GetBlockXSize() << "x" << band->GetBlockYSize() << std::endl;
    if (band->GetBlockXSize() != 64 || band->GetBlockYSize() != 64) {
      std::cerr << "ERROR: Custom block size not applied correctly for DS2 (expected 64x64)!" << std::endl;
    }

    double min_val, max_val, mean_val, std_dev_val;
    band->GetStatistics(TRUE, FALSE, &min_val, &max_val, &mean_val, &std_dev_val);
    std::cout << "Stats: Min=" << min_val << ", Max=" << max_val << ", Mean=" << mean_val << ", StdDev=" << std_dev_val << std::endl;

    unsigned char uc_pixel_value;
    band->ReadRaster(0, 0, 1, 1, &uc_pixel_value, 1, 1, GDT_Byte, 0, 0);
    std::cout << "Pixel (0,0) value: " << static_cast<int>(uc_pixel_value) << std::endl;
    
    unsigned char uc_pixel_value2;
    band->ReadRaster(500, 500, 1, 1, &uc_pixel_value2, 1, 1, GDT_Byte, 0, 0);
    std::cout << "Pixel (500,500) value: " << static_cast<int>(uc_pixel_value2) << std::endl;
  }

  GDALClose(ds2);
  std::cout << "Dataset 2 closed." << std::endl;


  std::cout << "\n--- Testing GDALOpen with JSON from a VSI memory file ---" << std::endl;
  const char* json_file_path = "/vsimem/random_raster_params.json";
  const char* json_file_content = R"({
    "type": "RANDOM_RASTER",
    "width": 150,
    "height": 150,
    "data_type": "Byte",
    "bits": 8,
    "seed": 112233,
    "geo_transform": [0.0, 10.0, 0.0, 0.0, 0.0, -10.0],
    "projection_wkt": "GEOGCS[\"GCS_North_American_1983\",DATUM[\"D_North_American_1983\",SPHEROID[\"GRS_1980\",6378137.0,298.257222101]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]]"
  })";

  VSIFCloseL(VSIFileFromMemBuffer(json_file_path, (GByte*)CPLStrdup(json_file_content), strlen(json_file_content), TRUE));

  GDALDataset* ds5 = (GDALDataset*)GDALOpen(json_file_path, GA_ReadOnly);

  if (ds5 == nullptr) {
      std::cerr << "Failed to open RANDOM_RASTER dataset from VSI memory file." << std::endl;
  } else {
      std::cout << "Dataset 5 (from VSI memory file) opened successfully." << std::endl;
      std::cout << "Raster size: " << ds5->GetRasterXSize() << "x" << ds5->GetRasterYSize() << std::endl;
      std::cout << "Band count: " << ds5->GetRasterCount() << std::endl;
      if (ds5->GetRasterCount() > 0) {
          GDALRasterBand* band = ds5->GetRasterBand(1);
          std::cout << "Band 1 data type: " << GDALGetDataTypeName(band->GetRasterDataType()) << std::endl;
          std::cout << "Band 1 block size: " << band->GetBlockXSize() << "x" << band->GetBlockYSize() << std::endl;
          if (band->GetBlockXSize() != 256 || band->GetBlockYSize() != 256) {
            std::cerr << "ERROR: Default block size not applied correctly for DS5 (expected 256x256)!" << std::endl;
          }
          double min_val, max_val, mean_val, std_dev_val;
          band->GetStatistics(TRUE, FALSE, &min_val, &max_val, &mean_val, &std_dev_val);
          std::cout << "Stats: Min=" << min_val << ", Max=" << max_val << ", Mean=" << mean_val << ", StdDev=" << std_dev_val << std::endl;
      }
      double gt5[6];
      ds5->GetGeoTransform(gt5);
      std::cout << "Retrieved GeoTransform: [";
      for (int i = 0; i < 6; ++i) {
          std::cout << gt5[i] << (i == 5 ? "" : ", ");
      }
      std::cout << "]" << std::endl;
      std::cout << "Retrieved Projection WKT: " << ds5->GetProjectionRef() << std::endl;

      GDALClose(ds5);
      std::cout << "Dataset 5 closed." << std::endl;
  }
  VSIUnlink(json_file_path);

  // Test cases for validation of geotransform
  std::cout << "\n--- Testing GDALOpen with invalid geotransform (zero pixel sizes) ---" << std::endl;
  const char* invalid_gt_json_str = R"({
    "type": "RANDOM_RASTER",
    "width": 10, "height": 10, "data_type": "Byte", "bits": 8,
    "geo_transform": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
  })";
  GDALDataset* ds_invalid_gt = (GDALDataset*)GDALOpen(invalid_gt_json_str, GA_ReadOnly);
  if (ds_invalid_gt == nullptr) {
      std::cout << "GDALOpen correctly returned nullptr for invalid geotransform (expected)." << std::endl;
  } else {
      std::cerr << "ERROR: GDALOpen opened a dataset with invalid geotransform!" << std::endl;
      GDALClose(ds_invalid_gt);
  }

  std::cout << "\n--- Testing GDALOpen with invalid type JSON (should return nullptr quietly) ---" << std::endl;
  const char* invalid_type_json_str = R"({"width": 100, "height": 100, "data_type": "Byte", "bits": 8, "type": "NOT_RANDOM_RASTER"})";
  GDALDataset* ds_invalid_type = (GDALDataset*)GDALOpen(invalid_type_json_str, GA_ReadOnly);
  if (ds_invalid_type == nullptr) {
      std::cout << "GDALOpen correctly returned nullptr for invalid type JSON (expected)." << std::endl;
  } else {
      std::cerr << "ERROR: GDALOpen opened a dataset with invalid type JSON!" << std::endl;
      GDALClose(ds_invalid_type);
  }

  std::cout << "\n--- Testing GDALOpen with malformed JSON string (should return nullptr quietly) ---" << std::endl;
  const char* malformed_json_str = R"({"width": 100, "height": 100)";
  GDALDataset* ds_malformed_json = (GDALDataset*)GDALOpen(malformed_json_str, GA_ReadOnly);
  if (ds_malformed_json == nullptr) {
      std::cout << "GDALOpen correctly returned nullptr for malformed JSON (expected)." << std::endl;
  } else {
      std::cerr << "ERROR: GDALOpen opened a dataset with malformed JSON!" << std::endl;
      GDALClose(ds_malformed_json);
  }

  std::cout << "\n--- Testing GDALOpen with valid JSON but missing required fields (should return nullptr with message) ---" << std::endl;
  const char* missing_fields_json_str = R"({"type": "RANDOM_RASTER", "width": 100})";
  GDALDataset* ds_missing_fields = (GDALDataset*)GDALOpen(missing_fields_json_str, GA_ReadOnly);
  if (ds_missing_fields == nullptr) {
      std::cout << "GDALOpen correctly returned nullptr for JSON with missing required fields (expected)." << std::endl;
  } else {
      std::cerr << "ERROR: GDALOpen opened a dataset with JSON missing required fields!" << std::endl;
      GDALClose(ds_missing_fields);
  }

  std::cout << "\n--- Testing GDALOpen with non-.json file extension (should return nullptr quietly) ---" << std::endl;
  const char* non_json_file_path = "/vsimem/not_json.txt";
  const char* non_json_file_content = "This is not JSON content.";
  VSIFCloseL(VSIFileFromMemBuffer(non_json_file_path, (GByte*)CPLStrdup(non_json_file_content), strlen(non_json_file_content), TRUE));

  GDALDataset* ds_non_json_file = (GDALDataset*)GDALOpen(non_json_file_path, GA_ReadOnly);
  if (ds_non_json_file == nullptr) {
      std::cout << "GDALOpen correctly returned nullptr for non-.json file (expected)." << std::endl;
  } else {
      std::cerr << "ERROR: GDALOpen opened a dataset with a non-.json file!" << std::endl;
      GDALClose(ds_non_json_file);
  }
  VSIUnlink(non_json_file_path);

  std::cout << "\n--- Testing GDALOpen with a .json file that is too large (should return nullptr quietly) ---" << std::endl;
  const char* large_json_file_path = "/vsimem/large_params.json";
  std::string large_json_content = "{\"type\":\"RANDOM_RASTER\",\"width\":1,\"height\":1,\"data_type\":\"Byte\",\"bits\":8}";
  for (int i = 0; i < (PRONTO_RASTER_MAX_JSON_FILE_SIZE / 100); ++i) {
      large_json_content += " ";
  }
  large_json_content += "}";

  VSIFCloseL(VSIFileFromMemBuffer(large_json_file_path, (GByte*)CPLStrdup(large_json_content.c_str()), strlen(large_json_content.c_str()), TRUE));

  GDALDataset* ds_large_file = (GDALDataset*)GDALOpen(large_json_file_path, GA_ReadOnly);
  if (ds_large_file == nullptr) {
      std::cout << "GDALOpen correctly returned nullptr for a large .json file (expected)." << std::endl;
  } else {
      std::cerr << "ERROR: GDALOpen opened a dataset from a too-large .json file!" << std::endl;
      GDALClose(ds_large_file);
  }
  VSIUnlink(large_json_file_path);

  GDALDestroyDriverManager();

  return 0;
}
#endif // GDAL_BUILD_AS_PLUGIN